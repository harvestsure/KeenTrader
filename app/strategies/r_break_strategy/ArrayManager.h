#pragma once

#include <engine/constant.h>
#include <engine/object.h>

using namespace Keen::engine;

class ArrayManager
{
public:
	ArrayManager(int size = 100);
	virtual ~ArrayManager();

	void update_bar(const BarData& bar);

	bool inited() const;

// 	def high()
// 	{
// 		/*
// 		Get high price time series.
// 		*/
// 		return self.high_array;
// 	}
// 
// 		def low(self)->np.ndarray:
// 		{
// 			/*
// 		Get low price time series.
// 		*/
// 			return self.low_array;
// 		}

public:
	std::tuple<float, float> donchian(int n);

private:
	int count = 0;
	int size = size;
	bool _inited = false;

	std::vector<double> high_array;
	std::vector<double> low_array;
};

