#pragma once

#include "interface/common.h"

#include <variant>
#include <array>

namespace render {


inline constexpr uint32_t kMaxRootParameters = 64;
inline constexpr uint32_t kMaxDescriptorRangesPerTable = 16;


enum class DescriptorType
{
	kCBV = 0,
	kSRV,
	kUAV
};


struct DescriptorRange
{
	DescriptorType descriptors_type = DescriptorType::kCBV;
	uint32_t base_shader_register = 0;
	uint32_t num_descriptors = 0;
};


class RootParameter
{
public:
	struct DescriptorTable
	{
		uint32_t num_ranges = 0;
		std::array<DescriptorRange, kMaxDescriptorRangesPerTable> ranges;

		DescriptorTable() = default;

		DescriptorTable(std::initializer_list<DescriptorRange> init)
		{
			ASSERT_MSG(init.size() <= kMaxDescriptorRangesPerTable, "Too many descriptor ranges in initializer list");
			std::copy_n(init.begin(), std::min(init.size(), static_cast<size_t>(kMaxDescriptorRangesPerTable)), ranges.begin());
			num_ranges = static_cast<uint32_t>(std::min(init.size(), static_cast<size_t>(kMaxDescriptorRangesPerTable)));
		}

		DescriptorRange& operator[](uint32_t index)
		{
			ASSERT_MSG(index < kMaxDescriptorRangesPerTable, "Descriptor range index out of range");
			num_ranges = std::max(num_ranges, index + 1);
			return ranges[index];
		}

		const DescriptorRange& operator[](uint32_t index) const
		{
			ASSERT_MSG(index < num_ranges, "Descriptor range index out of range");
			return ranges[index];
		}

		DescriptorTable& addRange(DescriptorType type, uint32_t base_shader_register, uint32_t num_descriptors)
		{
			ASSERT_MSG(num_ranges < kMaxDescriptorRangesPerTable, "Added max descriptor ranges to table");
			if (num_ranges < kMaxDescriptorRangesPerTable)
			{
				ranges[num_ranges++] = { type, base_shader_register, num_descriptors };
			}
			return *this;
		}
	};

	struct Constants32Bits
	{
		uint32_t shader_register = 0;
		uint32_t num_32bit_constants = 0;
		uint32_t register_space = 0;
	};

	struct Descriptor
	{
		DescriptorType type = DescriptorType::kCBV;
		uint32_t shader_register = 0;
		uint32_t register_space = 0;
	};

	static RootParameter makeDescriptorTable(std::initializer_list<DescriptorRange> init)
	{
		RootParameter param;
		param.parameter_data_ = DescriptorTable(init);
		return param;
	}

	static RootParameter makeDescriptorTable(const DescriptorTable& table = {})
	{
		RootParameter param;
		param.parameter_data_ = table;
		return param;
	}

	static RootParameter makeConstants32Bits(uint32_t shader_register, uint32_t num_constants, uint32_t register_space = 0)
	{
		RootParameter param;
		param.parameter_data_ = Constants32Bits{ shader_register, num_constants, register_space };
		return param;
	}

	static RootParameter makeCBVDescriptor(uint32_t shader_register, uint32_t register_space = 0)
	{
		RootParameter param;
		param.parameter_data_ = Descriptor{ DescriptorType::kCBV, shader_register, register_space };
		return param;
	}

	static RootParameter makeSRVDescriptor(uint32_t shader_register, uint32_t register_space = 0)
	{
		RootParameter param;
		param.parameter_data_ = Descriptor{ DescriptorType::kSRV, shader_register, register_space };
		return param;
	}

	static RootParameter makeUAVDescriptor(uint32_t shader_register, uint32_t register_space = 0)
	{
		RootParameter param;
		param.parameter_data_ = Descriptor{ DescriptorType::kUAV, shader_register, register_space };
		return param;
	}

	DescriptorTable& getAsDescriptorTable()
	{
		ASSERT_MSG(std::holds_alternative<DescriptorTable>(parameter_data_), "Root parameter is not a descriptor table");
		return std::get<DescriptorTable>(parameter_data_);
	}

	const DescriptorTable& getAsDescriptorTable() const
	{
		ASSERT_MSG(std::holds_alternative<DescriptorTable>(parameter_data_), "Root parameter is not a descriptor table");
		return std::get<DescriptorTable>(parameter_data_);
	}

	Constants32Bits& getAsConstants32Bits()
	{
		ASSERT_MSG(std::holds_alternative<Constants32Bits>(parameter_data_), "Root parameter is not a 32-bit constants");
		return std::get<Constants32Bits>(parameter_data_);
	}

	const Constants32Bits& getAsConstants32Bits() const
	{
		ASSERT_MSG(std::holds_alternative<Constants32Bits>(parameter_data_), "Root parameter is not a 32-bit constants");
		return std::get<Constants32Bits>(parameter_data_);
	}

	Descriptor& getAsDescriptor()
	{
		ASSERT_MSG(std::holds_alternative<Descriptor>(parameter_data_), "Root parameter is not a descriptor");
		return std::get<Descriptor>(parameter_data_);
	}

	const Descriptor& getAsDescriptor() const
	{
		ASSERT_MSG(std::holds_alternative<Descriptor>(parameter_data_), "Root parameter is not a descriptor");
		return std::get<Descriptor>(parameter_data_);
	}

	bool isDescriptorTable() const { return std::holds_alternative<DescriptorTable>(parameter_data_); }
	bool isConstants32Bits() const { return std::holds_alternative<Constants32Bits>(parameter_data_); }
	bool isCBVDescriptor() const { return std::holds_alternative<Descriptor>(parameter_data_) && std::get<Descriptor>(parameter_data_).type == DescriptorType::kCBV; }
	bool isSRVDescriptor() const { return std::holds_alternative<Descriptor>(parameter_data_) && std::get<Descriptor>(parameter_data_).type == DescriptorType::kSRV; }
	bool isUAVDescriptor() const { return std::holds_alternative<Descriptor>(parameter_data_) && std::get<Descriptor>(parameter_data_).type == DescriptorType::kUAV; }

	const auto& getVariant() const { return parameter_data_; }

private:
	std::variant<DescriptorTable, Constants32Bits, Descriptor> parameter_data_;
};


class ResourceLayoutState
{
public:
	RootParameter& operator[](uint32_t root_parameter_index)
	{
		ASSERT_MSG(root_parameter_index < kMaxRootParameters, "Root parameter index out of range");
		num_root_parameters_ = std::max(num_root_parameters_, root_parameter_index + 1);
		return root_parameters_[root_parameter_index];
	}

	const RootParameter& operator[](uint32_t root_parameter_index) const
	{
		ASSERT_MSG(root_parameter_index < num_root_parameters_, "Root parameter index out of range");
		return root_parameters_[root_parameter_index];
	}

	auto begin() { return root_parameters_.begin(); }
	auto end() { return root_parameters_.begin() + num_root_parameters_; }

	const auto begin() const { return root_parameters_.begin(); }
	const auto end() const { return root_parameters_.begin() + num_root_parameters_; }

	size_t getNumRootParameters() const { return num_root_parameters_; }

	// Make a hash of the resource layout state for use in pipeline state caching
	uint64_t makeHash() const
	{
		uint64_t hash = 0xcbf29ce484222325ULL; // FNV-1a offset basis
		auto combine = [&hash](uint64_t value) {
			hash ^= value;
			hash *= 0x100000001b3ULL; // FNV-1a prime
		};

		combine(num_root_parameters_);

		for (uint32_t i = 0; i < num_root_parameters_; ++i)
		{
			const auto& param = root_parameters_[i];

			if (auto* table = std::get_if<RootParameter::DescriptorTable>(&param.getVariant()))
			{
				combine(0); // variant index
				combine(table->num_ranges);
				for (uint32_t r = 0; r < table->num_ranges; ++r)
				{
					combine(static_cast<uint64_t>(table->ranges[r].descriptors_type));
					combine(table->ranges[r].base_shader_register);
					combine(table->ranges[r].num_descriptors);
				}
			}
			else if (auto* constants = std::get_if<RootParameter::Constants32Bits>(&param.getVariant()))
			{
				combine(1);
				combine(constants->shader_register);
				combine(constants->num_32bit_constants);
				combine(constants->register_space);
			}
			else if (auto* descriptor = std::get_if<RootParameter::Descriptor>(&param.getVariant()))
			{
				combine(2);
				combine(static_cast<uint64_t>(descriptor->type));
				combine(descriptor->shader_register);
				combine(descriptor->register_space);
			}
		}

		return hash;
	}

private:
	uint32_t num_root_parameters_ = 0;
	std::array<RootParameter, kMaxRootParameters> root_parameters_;
};

}  // namespace render