/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _TEST_CUDA_H_
#define _TEST_CUDA_H_

#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define LOWER_CASE_SUB_VAL ((char) ('A' - 'a'))

static inline void cpu_to_lower_case(char* str, int length)
{
    for(int i = 0; i < length; i++)
    {
        if(str[i] >= 'A' && str[i] <= 'Z')
        {
            str[i] -= LOWER_CASE_SUB_VAL;
        }
    }
}

void cuda_to_lower_case(char* str, int length, int deviceId);

void test_cuda_to_lower_case(int deviceId, char* str, int length, int gpu);

int get_cuda_device_id(void);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _TEST_CUDA_H_ */
