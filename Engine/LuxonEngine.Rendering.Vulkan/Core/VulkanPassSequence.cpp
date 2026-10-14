#include "vulkan-pch.h"
#include "VulkanPassSequence.h"
#include "VulkanTexture.h"
#include <Core/Logger.h>
#include <unordered_map>

namespace LuxonEngine::Rendering::Vulkan {
	namespace {
		struct ResourceUse
		{
			UInt32 passIndex;
			VulkanResourceState state;
		};

		struct StateChange
		{
			UInt32 passIndex;
			VulkanResourceState before;
			VulkanResourceState after;
		};

		struct TrackedResource
		{
			VkImage image = VK_NULL_HANDLE;
			VkBuffer buffer = VK_NULL_HANDLE;
			VulkanTexture* texture = nullptr;
			VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			VulkanResourceState initialState;
			std::vector<ResourceUse> uses;
		};

		bool IsSameState(const VulkanResourceState& a, const VulkanResourceState& b)
		{
			return a.layout == b.layout && a.stages == b.stages && a.access == b.access;
		}

		bool NeedsBarrier(const VulkanResourceState& current, const VulkanResourceState& required, VulkanResourceState& next)
		{
			next = required;

			if (current.layout == required.layout && IsReadOnlyState(current) && IsReadOnlyState(required)) {
				next.stages = current.stages | required.stages;
				next.access = current.access | required.access;
				return (required.stages & ~current.stages) != 0 || (required.access & ~current.access) != 0;
			}

			return true;
		}

		VulkanResourceState Simulate(const std::vector<ResourceUse>& uses, const VulkanResourceState& start, std::vector<StateChange>* changes)
		{
			VulkanResourceState current = start;

			for (auto& use : uses) {
				VulkanResourceState next;
				const bool needsBarrier = NeedsBarrier(current, use.state, next);

				if (needsBarrier && changes != nullptr)
					changes->push_back(StateChange{ use.passIndex, current, next });

				current = next;
			}

			return current;
		}

		template<typename Batch>
		void AddImageBarrier(Batch& batch, VkImage image, VkImageAspectFlags aspect, const VulkanResourceState& before, const VulkanResourceState& after)
		{
			batch.srcStages |= before.stages;
			batch.dstStages |= after.stages;
			batch.imageBarriers.push_back(VkImageMemoryBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = GetWriteAccess(before),
				.dstAccessMask = after.access,
				.oldLayout = before.layout,
				.newLayout = after.layout,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = { aspect, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS },
			});
		}

		template<typename Batch>
		void AddBufferBarrier(Batch& batch, VkBuffer buffer, const VulkanResourceState& before, const VulkanResourceState& after)
		{
			batch.srcStages |= before.stages;
			batch.dstStages |= after.stages;
			batch.bufferBarriers.push_back(VkBufferMemoryBarrier{
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				.srcAccessMask = GetWriteAccess(before),
				.dstAccessMask = after.access,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = buffer,
				.offset = 0,
				.size = VK_WHOLE_SIZE,
			});
		}

		template<typename Batch>
		void AddBarrier(Batch& batch, const TrackedResource& tracked, const VulkanResourceState& before, const VulkanResourceState& after)
		{
			if (tracked.image != VK_NULL_HANDLE)
				AddImageBarrier(batch, tracked.image, tracked.aspect, before, after);
			else
				AddBufferBarrier(batch, tracked.buffer, before, after);
		}

		template<typename Batch>
		void Submit(VkCommandBuffer commandBuffer, const Batch& batch)
		{
			if (batch.imageBarriers.empty() && batch.bufferBarriers.empty())
				return;

			vkCmdPipelineBarrier(commandBuffer,
				batch.srcStages != 0 ? batch.srcStages : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				batch.dstStages != 0 ? batch.dstStages : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0, 0, nullptr,
				(UInt32)batch.bufferBarriers.size(), batch.bufferBarriers.data(),
				(UInt32)batch.imageBarriers.size(), batch.imageBarriers.data());
		}
	}

	void VulkanPassSequence::AddPass(VulkanPass* pass)
	{
		if (pass != nullptr)
			m_passes.push_back(pass);

		m_compiled = false;
	}

	void VulkanPassSequence::SetBackBuffers(const std::vector<VkImage>& backBuffers)
	{
		m_backBuffers = backBuffers;
		m_compiled = false;
	}

	void VulkanPassSequence::Clear()
	{
		m_passes.clear();
		m_backBuffers.clear();
		m_passBarriers.clear();
		m_endBarriers.clear();
		m_prologueBarriers = {};
		m_entryStates.clear();
		m_compiled = false;
		m_prologueDone = false;
	}

	bool VulkanPassSequence::Compile()
	{
		m_compiled = false;
		m_prologueDone = false;
		m_passBarriers.clear();
		m_endBarriers.clear();
		m_prologueBarriers = {};
		m_entryStates.clear();

		const UInt32 backBufferCount = m_backBuffers.empty() ? 1 : (UInt32)m_backBuffers.size();
		const UInt32 passCount = (UInt32)m_passes.size();
		m_passBarriers.assign(backBufferCount, std::vector<BarrierBatch>(passCount));
		m_endBarriers.assign(backBufferCount, {});

		std::vector<TrackedResource> trackedResources;
		std::unordered_map<VkImage, size_t> imageIndices;
		std::unordered_map<VkBuffer, size_t> bufferIndices;
		std::vector<ResourceUse> backBufferUses;

		const VulkanResourceState unknownState{ VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_WRITE_BIT };

		for (UInt32 passIndex = 0; passIndex < passCount; passIndex++) {
			for (auto& usage : m_passes[passIndex]->GetUsages()) {
				size_t index = trackedResources.size();
				bool added;

				if (usage.image != VK_NULL_HANDLE) {
					auto result = imageIndices.try_emplace(usage.image, index);
					added = result.second;
					index = result.first->second;
				}
				else {
					auto result = bufferIndices.try_emplace(usage.buffer, index);
					added = result.second;
					index = result.first->second;
				}

				if (added) {
					VulkanResourceState initialState = unknownState;

					if (usage.texture != nullptr)
						initialState.layout = usage.texture->GetLayout();

					trackedResources.push_back(TrackedResource{
						.image = usage.image,
						.buffer = usage.buffer,
						.texture = usage.texture,
						.aspect = usage.aspect,
						.initialState = initialState,
					});
				}

				trackedResources[index].uses.push_back(ResourceUse{ passIndex, usage.state });
			}

			if (m_passes[passIndex]->UsesBackBuffer()) {
				if (m_backBuffers.empty()) {
					Logger::LogError("A pass uses the back buffer but the pass sequence has no back buffers");
					return false;
				}

				backBufferUses.push_back(ResourceUse{ passIndex, m_passes[passIndex]->GetBackBufferState() });
			}
		}

		for (auto& tracked : trackedResources) {
			VulkanResourceState entryState = tracked.initialState;
			bool settled = false;

			for (int iteration = 0; iteration < 4 && settled == false; iteration++) {
				VulkanResourceState endState = Simulate(tracked.uses, entryState, nullptr);
				settled = IsSameState(endState, entryState);
				entryState = endState;
			}

			if (settled == false)
				Logger::LogWarning("The states of a resource do not settle between frames, its barriers may be wrong");

			std::vector<StateChange> changes;
			Simulate(tracked.uses, entryState, &changes);

			if (IsSameState(tracked.initialState, entryState) == false)
				AddBarrier(m_prologueBarriers, tracked, tracked.initialState, entryState);

			if (tracked.texture != nullptr)
				m_entryStates.push_back(EntryState{ .texture = tracked.texture, .layout = entryState.layout });

			for (auto& change : changes) {
				for (auto& barriersOfBackBuffer : m_passBarriers)
					AddBarrier(barriersOfBackBuffer[change.passIndex], tracked, change.before, change.after);
			}
		}

		if (backBufferUses.empty() == false) {
			const VulkanResourceState acquiredState{ VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0 };
			const VulkanResourceState presentState{ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0 };

			std::vector<StateChange> changes;
			VulkanResourceState endState = Simulate(backBufferUses, acquiredState, &changes);

			for (UInt32 backBufferIndex = 0; backBufferIndex < backBufferCount; backBufferIndex++) {
				for (auto& change : changes)
					AddImageBarrier(m_passBarriers[backBufferIndex][change.passIndex], m_backBuffers[backBufferIndex], VK_IMAGE_ASPECT_COLOR_BIT, change.before, change.after);

				AddImageBarrier(m_endBarriers[backBufferIndex], m_backBuffers[backBufferIndex], VK_IMAGE_ASPECT_COLOR_BIT, endState, presentState);
			}
		}

		m_compiled = true;
		return true;
	}

	void VulkanPassSequence::Execute(VkCommandBuffer commandBuffer, UInt32 backBufferIndex)
	{
		if (m_compiled == false) {
			Logger::LogError("The pass sequence is not compiled, the passes are not executed");
			return;
		}

		if (backBufferIndex >= m_passBarriers.size())
			backBufferIndex = 0;

		if (m_prologueDone == false) {
			Submit(commandBuffer, m_prologueBarriers);

			for (auto& entry : m_entryStates)
				entry.texture->SetLayout(entry.layout);

			m_prologueDone = true;
		}

		for (UInt32 passIndex = 0; passIndex < m_passes.size(); passIndex++) {
			Submit(commandBuffer, m_passBarriers[backBufferIndex][passIndex]);
			m_passes[passIndex]->Execute(commandBuffer);
		}

		Submit(commandBuffer, m_endBarriers[backBufferIndex]);
	}
}
