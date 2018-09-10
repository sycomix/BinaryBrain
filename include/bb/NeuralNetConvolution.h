// --------------------------------------------------------------------------
//  Binary Brain  -- binary neural net framework
//
//                                     Copyright (C) 2018 by Ryuji Fuchikami
//                                     https://github.com/ryuz
//                                     ryuji.fuchikami@nifty.com
// --------------------------------------------------------------------------



#pragma once


#include <vector>
#include <random>
#include <intrin.h>
#include "NeuralNetLayerBuf.h"


namespace bb {


// Convolutionクラス
template <typename T = float, typename INDEX = size_t>
class NeuralNetConvolution : public NeuralNetLayerBuf<T, INDEX>
{
protected:
	INDEX			m_mux_size = 1;
	INDEX			m_frame_size = 1;
	int				m_input_h_size;
	int				m_input_w_size;
	int				m_input_c_size;
	int				m_filter_h_size;
	int				m_filter_w_size;
	int				m_output_h_size;
	int				m_output_w_size;
	int				m_output_c_size;
	std::vector <T>	m_W;
	std::vector <T>	m_b;
	std::vector <T>	m_dW;
	std::vector <T>	m_db;

public:
	NeuralNetConvolution() {}
	
	NeuralNetConvolution(INDEX input_c_size, INDEX input_h_size, INDEX input_w_size, INDEX output_c_size, INDEX filter_h_size, INDEX filter_w_size, std::uint64_t seed = 1)
	{
		Resize(input_c_size, input_h_size, input_w_size, output_c_size, filter_h_size, filter_w_size, seed);
	}
	
	~NeuralNetConvolution() {}		// デストラクタ
	
	void Resize(INDEX input_c_size, INDEX input_h_size, INDEX input_w_size, INDEX output_c_size, INDEX filter_h_size, INDEX filter_w_size, std::uint64_t seed = 1)
	{
		m_input_c_size = (int)input_c_size;
		m_input_h_size = (int)input_h_size;
		m_input_w_size = (int)input_w_size;
		m_filter_h_size = (int)filter_h_size;
		m_filter_w_size = (int)filter_w_size;
		m_output_c_size = (int)output_c_size;
		m_output_h_size = m_input_h_size - m_filter_h_size + 1;
		m_output_w_size = m_input_w_size - m_filter_w_size + 1;

		m_W.resize(m_output_c_size*m_input_c_size*m_filter_h_size*m_filter_w_size);
		m_b.resize(m_output_c_size);
		m_dW.resize(m_output_c_size*m_input_c_size*m_filter_h_size*m_filter_w_size);
		m_db.resize(m_output_c_size);

		std::mt19937_64 mt(seed);
		std::uniform_real_distribution<T> uniform_rand((T)0, (T)1);
		for (auto& w : m_W) {
			w = uniform_rand(mt);
		}
		for (auto& b : m_b) {
			b = uniform_rand(mt);
		}
	}
	
	T& W(INDEX n, INDEX c, INDEX y, INDEX x) {
		BB_ASSERT(n >= 0 && n < m_output_c_size);
		BB_ASSERT(c >= 0 && c < m_input_c_size);
		BB_ASSERT(y >= 0 && y < m_input_h_size);
		BB_ASSERT(x >= 0 && x < m_input_w_size);
		return m_W[((n*m_input_c_size + c)*m_filter_h_size + y)*m_filter_w_size + x];
	}

	T& b(INDEX n) {
		BB_ASSERT(n >= 0 && n < m_output_c_size);
		return m_b[n];
	}

	T& dW(INDEX n, INDEX c, INDEX y, INDEX x) {
		BB_ASSERT(n >= 0 && n < m_output_c_size);
		BB_ASSERT(c >= 0 && c < m_input_c_size);
		BB_ASSERT(y >= 0 && y < m_input_h_size);
		BB_ASSERT(x >= 0 && x < m_input_w_size);
		return m_dW[((n*m_input_c_size + c)*m_filter_h_size + y)*m_filter_w_size + x];
	}

	T& db(INDEX n) {
		BB_ASSERT(n >= 0 && n < m_output_c_size);
		return m_db[n];
	}
	
	void SetMuxSize(INDEX mux_size) { m_mux_size = mux_size; }

	void SetBatchSize(INDEX batch_size) {
		m_frame_size = batch_size * m_mux_size;
	}
	
	INDEX GetInputFrameSize(void) const { return m_frame_size; }
	INDEX GetInputNodeSize(void) const { return m_input_c_size * m_input_h_size * m_input_w_size; }
	INDEX GetOutputFrameSize(void) const { return m_frame_size; }
	INDEX GetOutputNodeSize(void) const { return m_output_c_size * m_output_h_size * m_output_w_size; }
	
	int   GetInputValueDataType(void) const { return NeuralNetType<T>::type; }
	int   GetInputErrorDataType(void) const { return NeuralNetType<T>::type; }
	int   GetOutputValueDataType(void) const { return NeuralNetType<T>::type; }
	int   GetOutputErrorDataType(void) const { return NeuralNetType<T>::type; }
	
protected:

	inline T* GetInputPtr(NeuralNetBuffer<T, INDEX>& buf, int c, int y, int x)
	{
		return (T*)buf.GetPtr((c*m_input_h_size + y)*m_input_w_size + x);
	}

	inline T* GetOutputPtr(NeuralNetBuffer<T, INDEX>& buf, int c, int y, int x)
	{
		return (T*)buf.GetPtr((c*m_output_h_size + y)*m_output_w_size + x);
	}

	inline T* GetOutputPtrWithRangeCheck(NeuralNetBuffer<T, INDEX>& buf, int c, int y, int x)
	{
		if (x < 0 || x >= m_output_w_size || y < 0 || y >= m_output_h_size) {
			return (T*)buf.GetZeroPtr();
		}

		return (T*)buf.GetPtr((c*m_output_h_size + y)*m_output_w_size + x);
	}

	inline T* GetWPtr(NeuralNetBuffer<T, INDEX>& buf, INDEX n, INDEX c, INDEX y, INDEX x)
	{
		BB_ASSERT(n >= 0 && n < m_output_c_size);
		BB_ASSERT(c >= 0 && c < m_input_c_size);
		BB_ASSERT(y >= 0 && y < m_input_h_size);
		BB_ASSERT(x >= 0 && x < m_input_w_size);
		return (T*)buf.GetPtr(((n*m_input_c_size + c)*m_filter_h_size + y)*m_filter_w_size + x);
	}

	inline float my_mm256_sum_ps(__m256 r)
	{
//		return r.m256_f32[0] + r.m256_f32[1] + r.m256_f32[2] + r.m256_f32[3]
//			+ r.m256_f32[4] + r.m256_f32[5] + r.m256_f32[6] + r.m256_f32[7];
		r = _mm256_hadd_ps(r, r);
		r = _mm256_hadd_ps(r, r);
		return r.m256_f32[0] + r.m256_f32[4];
	}

public:
	void Forward(bool train = true)
	{
		if (typeid(T) == typeid(float)) {
			// float用実装
			int  m256_frame_size = (int)(((m_frame_size + 7) / 8) * 8);
			auto in_buf = GetInputValueBuffer();
			auto out_buf = GetOutputValueBuffer();

			for (int n = 0; n < m_output_c_size; ++n) {
				for (int y = 0; y < m_output_h_size; ++y) {
					for (int x = 0; x < m_output_w_size; ++x) {
						float* out_ptr = GetOutputPtr(out_buf, n, y, x);
						for (size_t frame = 0; frame < m256_frame_size; frame += 8) {
							__m256 sum = _mm256_set1_ps(b(n));
							for (int c = 0; c < m_input_c_size; ++c) {
								for (int fy = 0; fy < m_filter_h_size; ++fy) {
									for (int fx = 0; fx < m_filter_w_size; ++fx) {
										int ix = x + fx;
										int iy = y + fy;
										float* in_ptr = GetInputPtr(in_buf, c, iy, ix);
										__m256 W_val = _mm256_set1_ps(W(n, c, fy, fx));
										__m256 in_val = _mm256_load_ps(&in_ptr[frame]);
										__m256 mul_val = _mm256_mul_ps(W_val, in_val);
										sum = _mm256_add_ps(sum, mul_val);
									}
								}
							}
							_mm256_store_ps(&out_ptr[frame], sum);
						}
					}
				}
			}
		}
		else if (typeid(T) == typeid(double)) {
			// double用実装
		}
		else {
			assert(0);
		}
	}
	
	void Backward(void)
	{
		if (typeid(T) == typeid(float)) {
			// float用実装
			int  m256_frame_size = (int)(((m_frame_size + 7) / 8) * 8);
			auto in_val_buf = GetInputValueBuffer();
			auto out_val_buf = GetOutputValueBuffer();
			auto in_err_buf = GetInputErrorBuffer();
			auto out_err_buf = GetOutputErrorBuffer();

			// パラメータdWの計算
			for (int n = 0; n < m_output_c_size; ++n) {
				__m256 sum_db = _mm256_set1_ps(0);
				for (int c = 0; c < m_input_c_size; ++c) {
					for (int fy = 0; fy < m_filter_h_size; ++fy) {
						for (int fx = 0; fx < m_filter_w_size; ++fx) {
							__m256 sum_dW = _mm256_set1_ps(0);
							for (int y = 0; y < m_output_h_size; ++y) {
								for (int x = 0; x < m_output_w_size; ++x) {
									int ix = x + fx;
									int iy = y + fy;
									float* out_err_ptr = GetOutputPtr(out_err_buf, n, y, x);
									float* in_val_ptr = GetInputPtr(in_val_buf, c, iy, ix);
									for (size_t frame = 0; frame < m256_frame_size; frame += 8) {
										__m256 out_err = _mm256_load_ps(&out_err_ptr[frame]);
										__m256 in_val = _mm256_load_ps(&in_val_ptr[frame]);
										__m256 mul_val = _mm256_mul_ps(in_val, out_err);
										sum_dW = _mm256_add_ps(sum_dW, mul_val);
									}
								}
							}
							dW(n, c, fy, fx) = my_mm256_sum_ps(sum_dW);
						}
					}
				}
			}

			// パラメータdbの計算
			for (int n = 0; n < m_output_c_size; ++n) {
				__m256 sum_db = _mm256_set1_ps(0);
				for (int y = 0; y < m_output_h_size; ++y) {
					for (int x = 0; x < m_output_w_size; ++x) {
						float* out_err_ptr = GetOutputPtr(out_err_buf, n, y, x);
						for (size_t frame = 0; frame < m256_frame_size; frame += 8) {
							__m256 out_err = _mm256_load_ps(&out_err_ptr[frame]);
							sum_db = _mm256_add_ps(sum_db, out_err);
						}
					}
				}
				db(n) = my_mm256_sum_ps(sum_db);
			}


			// 入力への逆伝播
			for (int c = 0; c < m_input_c_size; ++c) {
				for (int y = 0; y < m_input_h_size; ++y) {
					for (int x = 0; x < m_input_w_size; ++x) {
						float* in_err_ptr = GetInputPtr(in_err_buf, c, y, x);
						for (size_t frame = 0; frame < m256_frame_size; frame += 8) {
							__m256 sum = _mm256_set1_ps(0);
							for (int n = 0; n < m_output_c_size; ++n) {
								for (int fy = 0; fy < m_filter_h_size; ++fy) {
									for (int fx = 0; fx < m_filter_w_size; ++fx) {
										int ox = x - fx;
										int oy = y - fy;
										float* out_err_ptr = GetOutputPtrWithRangeCheck(out_err_buf, n, oy, ox);
										__m256 W_val = _mm256_set1_ps(W(n, c, fy, fx));
										__m256 out_err = _mm256_load_ps(&out_err_ptr[frame]);
										__m256 mul_val = _mm256_mul_ps(W_val, out_err);
										sum = _mm256_add_ps(sum, mul_val);
									}
								}
							}
							_mm256_store_ps(&in_err_ptr[frame], sum);
						}
					}
				}
			}
		}
	}

	void Update(double learning_rate)
	{
	}

};


}