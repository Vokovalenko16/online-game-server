#pragma once

namespace mserver {

	struct ProgressRange
	{
		float start, length;

		ProgressRange() = default;

		ProgressRange(float start, float length) : start(start), length(length) {}
	};

	class ProgressDivider
	{
	private:
		ProgressRange total_range_;
		float total_weight_;
		float current_pos_;

	public:
		ProgressDivider(const ProgressRange& range, float weight)
			: total_range_(range)
			, total_weight_(weight)
			, current_pos_(0)
		{
		}

		ProgressRange divide(float weight)
		{
			auto result = ProgressRange{
				total_range_.start + total_range_.length * (current_pos_ / total_weight_),
				total_range_.length * (weight / total_weight_)
			};

			current_pos_ += weight;
			return result;
		}
	};
}