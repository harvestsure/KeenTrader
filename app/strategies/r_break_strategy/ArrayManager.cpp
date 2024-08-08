#include <api/Globals.h>
#include "ArrayManager.h"
//#include <indicators.h>

ArrayManager::ArrayManager(int size/* = 100*/)
	: count(0)
	, size(size)
	, _inited(false)
{
	high_array.resize(size, 0);
	low_array.resize(size, 0);

// 	open_array: np.ndarray = np.zeros(size);
// 	high_array: np.ndarray = np.zeros(size);
// 	low_array: np.ndarray = np.zeros(size);
// 	close_array: np.ndarray = np.zeros(size);
// 	volume_array: np.ndarray = np.zeros(size);
// 	open_interest_array: np.ndarray = np.zeros(size);
}

ArrayManager::~ArrayManager()
{

}

void ArrayManager::update_bar(const BarData& bar)
{
	/*
	Update new bar data into array manager.
	*/

	this->count += 1;
	if (!this->_inited && this->count >= this->size)
		this->_inited = true;

	high_array.erase(high_array.begin());
	high_array.push_back(bar.high_price);

	low_array.erase(low_array.begin());
	low_array.push_back(bar.low_price);

// 	this->open_array[:-1] = this->open_array[1:];
// 	this->high_array[:-1] = this->high_array[1:];
// 	this->low_array[:-1] = this->low_array[1:];
// 	this->close_array[:-1] = this->close_array[1:];
// 	this->volume_array[:-1] = this->volume_array[1:];
// 	this->open_interest_array[:-1] = this->open_interest_array[1:];
// 
// 	this->open_array[-1] = bar.open_price;
// 	this->high_array[-1] = bar.high_price;
// 	this->low_array[-1] = bar.low_price;
// 	this->close_array[-1] = bar.close_price;
// 	this->volume_array[-1] = bar.volume;
// 	this->open_interest_array[-1] = bar.open_interest;

}

bool ArrayManager::inited() const
{
	return _inited;
}

std::tuple<float, float> ArrayManager::donchian(int n)
{
	/*
	Donchian Channel.
	*/
	/*

	TI_REAL options[] = { n };

	int out_size = ti_max_start(options);
	TI_REAL *high_inputs[] = { high_array.data() };
	std::vector<TI_REAL> high_output_arr(out_size, 0);
	TI_REAL *high_outputs[1] = { high_output_arr.data() };

	ti_max(high_array.size(), high_inputs, options, high_outputs);

	out_size = ti_min_start(options);
	TI_REAL *low_inputs[] = { low_array.data() };
	std::vector<TI_REAL> low_output_arr(out_size, 0);
	TI_REAL *low_outputs[1] = { low_output_arr.data() };

	ti_min(low_array.size(), low_inputs, options, low_outputs);

	return std::make_tuple(*high_output_arr.rbegin(), *low_output_arr.rbegin());
	*/
	return std::make_tuple(0.f, 0.f);
}
