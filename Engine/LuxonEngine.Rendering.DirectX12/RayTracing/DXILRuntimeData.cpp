#include "pch.h"
#include "DXILRuntimeData.h"
#include <cstring>

using namespace Microsoft::WRL;

namespace LuxonEngine::Rendering::DX12::RayTracing {
	namespace {
		// the first fields of a function record: name, unmangled name, resources, function dependencies, shader kind, payload size, attribute size.
		// the record can have more fields after them, the stride of the table is used to step over them
		constexpr UInt32 FunctionFieldCount = 7;

		// the type of the string buffer part. the numbers of the other parts are different in the versions of the compiler
		constexpr UInt32 StringBufferPart = 1;

		struct Span {
			const Byte* data = nullptr;
			UInt64 size = 0;
		};

		UInt32 ReadUInt32(const Byte* data, UInt64 offset)
		{
			UInt32 value;
			std::memcpy(&value, data + offset, sizeof(UInt32));
			return value;
		}

		// reads a null terminated string of the string buffer
		bool ReadString(const Span& strings, UInt32 offset, std::string& value)
		{
			if (offset >= strings.size)
				return false;

			const char* start = reinterpret_cast<const char*>(strings.data) + offset;
			size_t length = strnlen(start, strings.size - offset);

			if (length == strings.size - offset)
				return false;

			value.assign(start, length);
			return true;
		}

		// reads a table part as the function table. the part is accepted only if the unmangled name of every record is inside its mangled name,
		// which the records of the other tables, such as the resources, do not satisfy
		bool ReadFunctionTable(const Span& table, const Span& strings, std::vector<DXILFunctionData>& functions)
		{
			if (table.size < 8)
				return false;

			// table: the number of records, the stride of a record. then the records
			UInt32 recordCount = ReadUInt32(table.data, 0);
			UInt32 recordStride = ReadUInt32(table.data, 4);

			if (recordCount == 0 || recordStride < FunctionFieldCount * sizeof(UInt32) || 8 + (UInt64)recordCount * recordStride > table.size)
				return false;

			std::vector<DXILFunctionData> tableFunctions;

			for (UInt32 i = 0; i < recordCount; i++) {
				const Byte* record = table.data + 8 + (UInt64)i * recordStride;
				DXILFunctionData function;

				if (ReadString(strings, ReadUInt32(record, 0), function.mangledName) == false
					|| ReadString(strings, ReadUInt32(record, 4), function.name) == false
					|| function.name.empty())
					return false;

				if (function.mangledName != function.name && function.mangledName.find("?" + function.name + "@@") == std::string::npos)
					return false;

				function.shaderKind = ReadUInt32(record, 16);
				function.payloadSize = ReadUInt32(record, 20);
				function.attributeSize = ReadUInt32(record, 24);
				tableFunctions.push_back(std::move(function));
			}

			functions = std::move(tableFunctions);
			return true;
		}
	}

	bool ReadDXILFunctions(IDxcBlob* library, std::vector<DXILFunctionData>& functions)
	{
		functions.clear();

		ComPtr<IDxcContainerReflection> containerReflection;
		ComPtr<IDxcBlob> runtimeDataBlob;
		UInt32 partIndex = 0;

		if (library == nullptr
			|| FAILED(DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&containerReflection)))
			|| FAILED(containerReflection->Load(library))
			|| FAILED(containerReflection->FindFirstPartKind(DXC_FOURCC('R', 'D', 'A', 'T'), &partIndex))
			|| FAILED(containerReflection->GetPartContent(partIndex, &runtimeDataBlob)))
			return false;

		const Byte* data = static_cast<const Byte*>(runtimeDataBlob->GetBufferPointer());
		UInt64 size = runtimeDataBlob->GetBufferSize();

		// header: version, part count. then the offsets of the parts. a part has a header with its type and its size
		if (size < 8)
			return false;

		UInt32 partCount = ReadUInt32(data, 4);
		Span strings;
		std::vector<Span> tables;

		for (UInt32 i = 0; i < partCount; i++) {
			if (8 + (UInt64)(i + 1) * sizeof(UInt32) > size)
				return false;

			UInt32 partOffset = ReadUInt32(data, 8 + (UInt64)i * sizeof(UInt32));

			if ((UInt64)partOffset + 8 > size || (UInt64)partOffset + 8 + ReadUInt32(data, partOffset + 4ull) > size)
				return false;

			Span part{ data + partOffset + 8, ReadUInt32(data, partOffset + 4ull) };

			if (ReadUInt32(data, partOffset) == StringBufferPart)
				strings = part;
			else
				tables.push_back(part);
		}

		if (strings.data == nullptr)
			return false;

		for (auto& table : tables) {
			if (ReadFunctionTable(table, strings, functions))
				return true;
		}

		return false;
	}
}
