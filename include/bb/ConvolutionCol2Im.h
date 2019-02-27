﻿// --------------------------------------------------------------------------
//  Binary Brain  -- binary neural net framework
//
//                                 Copyright (C) 2018-2019 by Ryuji Fuchikami
//                                 https://github.com/ryuz
//                                 ryuji.fuchikami@nifty.com
// --------------------------------------------------------------------------


#pragma once


#include <vector>
#include <random>

#include "bb/Layer.h"


namespace bb {


template <typename FT = float, typename BT = float>
class ConvolutionCol2Im : public Layer
{
protected:
	index_t			m_c_size;
	index_t			m_h_size;
	index_t			m_w_size;
    
    FrameBuffer     m_y;
    FrameBuffer     m_dx;
    
protected:
	ConvolutionCol2Im() {}

public:
	~ConvolutionCol2Im() {}

    struct create_t
    {
        index_t c_size = 1;
        index_t h_size = 1;
        index_t w_size = 1;
    };

	static std::shared_ptr<ConvolutionCol2Im> Create(create_t const & create)
	{
        auto self = std::shared_ptr<ConvolutionCol2Im>(new ConvolutionCol2Im);
		self->m_c_size = create.c_size;
        self->m_h_size = create.h_size;
        self->m_w_size = create.w_size;
        return self;
	}

    static std::shared_ptr<ConvolutionCol2Im> Create(index_t c_size, index_t h_size, index_t w_size)
	{
        auto self = std::shared_ptr<ConvolutionCol2Im>(new ConvolutionCol2Im);
		self->m_c_size = c_size;
        self->m_h_size = h_size;
        self->m_w_size = w_size;
        return self;
	}
	
	std::string GetClassName(void) const { return "ConvolutionCol2Im"; }

	int GetChannel(void) const { return m_c_size; }
	int GetHeight(void)  const { return m_h_size; }
	int GetWidth(void)   const { return m_w_size; }
	

public:
    FrameBuffer Forward(FrameBuffer x, bool train=true)
 	{
        BB_ASSERT(x.GetType() == DataType<FT>::type);

       	index_t input_frame_size  = x.GetFrameSize();
        BB_ASSERT(input_frame_size % (m_h_size * m_w_size) == 0);
    	index_t output_frame_size = input_frame_size / (m_h_size * m_w_size);

        m_y.Resize(DataType<FT>::type, output_frame_size, indices_t({m_w_size, m_h_size, m_c_size}));

        {
            auto x_ptr = x.GetConstPtr<FT>();
            auto y_ptr = m_y.GetPtr<FT>(true);
		    index_t input_frame = 0;
		    for (index_t output_frame = 0; output_frame < output_frame_size; ++output_frame) {
			    for (index_t y = 0; y < m_h_size; ++y) {
				    for (index_t x = 0; x < m_w_size; ++x) {
					    #pragma omp parallel for
					    for (index_t c = 0; c < m_c_size; ++c) {
						    index_t input_node = c;
						    index_t output_node = (c*m_h_size + y)*m_w_size + x;
						    y_ptr.Set(output_frame, output_node, x_ptr.Get(input_frame, input_node));
					    }
					    ++input_frame;
				    }
			    }
		    }
            return m_y;
        }
	}
	
	FrameBuffer Backward(FrameBuffer dy)
	{
        BB_ASSERT(dy.GetType() == DataType<BT>::type);

    	index_t output_frame_size = dy.GetFrameSize();
       	index_t input_frame_size  = output_frame_size *(m_h_size * m_w_size);

        m_dx.Resize(DataType<BT>::type, input_frame_size, m_c_size);
        
        {
		    auto dy_ptr = dy.GetConstPtr<BT>();
		    auto dx_ptr = m_dx.GetPtr<BT>(true);

		    index_t input_frame = 0;
		    for (index_t output_frame = 0; output_frame < output_frame_size; ++output_frame) {
			    for (index_t y = 0; y < m_h_size; ++y) {
				    for (index_t x = 0; x < m_w_size; ++x) {
					    #pragma omp parallel for
					    for (index_t c = 0; c < m_c_size; ++c) {
						    index_t output_node = (c*m_h_size + y)*m_w_size + x;
						    index_t input_node = c;
						    dx_ptr.Set(input_frame, input_node, dy_ptr.Get(output_frame, output_node));
					    }
					    ++input_frame;
				    }
			    }
		    }
            return m_dx;
        }
	}
};


}