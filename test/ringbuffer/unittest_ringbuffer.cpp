#include <thread>
#include <chrono>
#include <vector>
#include "gtest/gtest.h"
#include "muggle/c/muggle_c.h"

TEST(ringbuffer, usage_utils)
{
	muggle_atomic_int capacity = 1 << 10;
	muggle_atomic_int pos = 1 << ((sizeof(muggle_atomic_int) * 8) - 1);
	pos = ~pos;

	ASSERT_GT(pos, 0);
	ASSERT_LT(pos + 1, 0);

	muggle_atomic_int idx_in_ring = IDX_IN_POW_OF_2_RING(pos, capacity);
	ASSERT_GE(idx_in_ring, 0);
	ASSERT_LT(idx_in_ring, capacity);
	if (idx_in_ring == capacity - 1)
	{
		ASSERT_EQ(IDX_IN_POW_OF_2_RING(pos+1, capacity), 0);
	}
	else
	{
		ASSERT_EQ(IDX_IN_POW_OF_2_RING(pos+1, capacity), idx_in_ring + 1);
	}

	int k = 8;
	pos = pos - capacity * k - 1;
	muggle_atomic_int last_idx_in_ring = IDX_IN_POW_OF_2_RING(pos, capacity);
	++pos;
	for (muggle_atomic_int i = 0; i < capacity * k * 2; ++i)
	{
		idx_in_ring = IDX_IN_POW_OF_2_RING(pos, capacity);
		ASSERT_GE(idx_in_ring, 0);
		ASSERT_LT(idx_in_ring, capacity);
		if (last_idx_in_ring == capacity - 1)
		{
			ASSERT_EQ(idx_in_ring, 0);
		}
		else
		{
			ASSERT_EQ(idx_in_ring, last_idx_in_ring + 1);
		}
		last_idx_in_ring = idx_in_ring;
		++pos;
	}
}

TEST(ringbuffer, init_destroy)
{
	muggle_ringbuffer_t r;
	muggle_atomic_int capacity = 16;
	int ret;

	ret = muggle_ringbuffer_init(&r, capacity, 0);
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(r.capacity, capacity);

	ret = muggle_ringbuffer_destroy(&r);
	EXPECT_EQ(ret, 0);
}

TEST(ringbuffer, push_get_in_single_thread)
{
	muggle_ringbuffer_t r;
	muggle_atomic_int capacity = 16;
	muggle_atomic_int pos = 0;
	int arr[160];
	int flag = 
		MUGGLE_RINGBUFFER_FLAG_SINGLE_WRITER |
		MUGGLE_RINGBUFFER_FLAG_SINGLE_READER;

	muggle_ringbuffer_init(&r, capacity, flag);

	// push capacity / 2, and get all
	for (muggle_atomic_int i = 0; i < capacity / 2; ++i)
	{
		arr[i] = (int)i;
		muggle_ringbuffer_write(&r, &arr[i]);
	}
	for (muggle_atomic_int i = 0; i < capacity / 2; ++i)
	{
		int data = *(int*)muggle_ringbuffer_read(&r, pos++);
		EXPECT_EQ(data, (int)i);
	}

	// push one and get one
	for (muggle_atomic_int i = 0; i < capacity * 5; ++i)
	{
		arr[i] = (int)i;
		muggle_ringbuffer_write(&r, &arr[i]);

		int data = *(int*)muggle_ringbuffer_read(&r, pos++);
		EXPECT_EQ(data, (int)i);
	}

	muggle_ringbuffer_destroy(&r);
}

TEST(ringbuffer, st_push_get_in_single_thread)
{
	muggle_ringbuffer_t r;
	muggle_atomic_int capacity = 16;
	muggle_atomic_int pos = 0;
	int arr[160];
	int flag = 
		MUGGLE_RINGBUFFER_FLAG_SINGLE_WRITER |
		MUGGLE_RINGBUFFER_FLAG_SINGLE_READER;

	muggle_ringbuffer_init(&r, capacity, flag);

	// push capacity / 2, and get all
	for (muggle_atomic_int i = 0; i < capacity / 2; ++i)
	{
		arr[i] = (int)i;
		muggle_ringbuffer_write(&r, &arr[i]);
	}
	for (muggle_atomic_int i = 0; i < capacity / 2; ++i)
	{
		int data = *(int*)muggle_ringbuffer_read(&r, pos++);
		EXPECT_EQ(data, (int)i);
	}

	// push one and get one
	for (muggle_atomic_int i = 0; i < capacity * 5; ++i)
	{
		arr[i] = (int)i;
		muggle_ringbuffer_write(&r, &arr[i]);

		int data = *(int*)muggle_ringbuffer_read(&r, pos++);
		EXPECT_EQ(data, (int)i);
	}

	muggle_ringbuffer_destroy(&r);
}


TEST(ringbuffer, one_producer_one_consumer)
{
	muggle_ringbuffer_t r;
	muggle_atomic_int capacity = 1024 * 16;
	muggle_atomic_int cnt = 1024 * 100;
	int *arr = (int*)malloc(sizeof(int) * cnt);
	int end_flag = -1;
	for (int i = 0; i < cnt; ++i)
	{
		arr[i] = i;
	}
	muggle_atomic_int consumer_ready = 0;

	int flag = 
		MUGGLE_RINGBUFFER_FLAG_SINGLE_WRITER |
		MUGGLE_RINGBUFFER_FLAG_SINGLE_READER;
	muggle_ringbuffer_init(&r, capacity, flag);

	std::thread consumer([&]{
		muggle_atomic_store(&consumer_ready, 1, muggle_memory_order_relaxed);

		muggle_atomic_int pos = 0;
		int recv_idx = 0;

		while (1)
		{
			void *data = muggle_ringbuffer_read(&r, pos++);
			if (data == &end_flag)
			{
				break;
			}
			EXPECT_GE(*(int*)data, recv_idx);
			if (*(int*)data != recv_idx)
			{
				// message loss
				recv_idx = *(int*)data;
			}
			++recv_idx;
		}
	});

	std::thread producer([&]{
		while (muggle_atomic_load(&consumer_ready, muggle_memory_order_relaxed) != 1);

		for (int i = 0; i < cnt; ++i)
		{
			int ret = muggle_ringbuffer_write(&r, &arr[i]);
			EXPECT_EQ(ret, (int)MUGGLE_OK);
		}
		muggle_ringbuffer_write(&r, &end_flag);
	});

	producer.join();
	consumer.join();

	muggle_ringbuffer_destroy(&r);
	free(arr);
}

TEST(ringbuffer, one_producer_mul_consumer)
{
	muggle_ringbuffer_t r;
	muggle_atomic_int capacity = 1024 * 16;
	muggle_atomic_int cnt = 1024 * 100;
	int *arr = (int*)malloc(sizeof(int) * cnt);
	int end_flag = -1;
	int consumer_num = 5;
	for (int i = 0; i < cnt; ++i)
	{
		arr[i] = i;
	}
	muggle_atomic_int consumer_ready = 0;

	int flag = MUGGLE_RINGBUFFER_FLAG_SINGLE_WRITER;
	muggle_ringbuffer_init(&r, capacity, flag);

	std::vector<std::thread> consumers;
	for (int i = 0; i < consumer_num; ++i)
	{
		consumers.push_back(std::thread([&]{
			muggle_atomic_fetch_add(&consumer_ready, 1, muggle_memory_order_relaxed);

			muggle_atomic_int pos = 0;
			int recv_idx = 0;

			while (1)
			{
				void *data = muggle_ringbuffer_read(&r, pos++);
				if (data == &end_flag)
				{
					break;
				}
				EXPECT_GE(*(int*)data, recv_idx);
				if (*(int*)data != recv_idx)
				{
					// message loss
					recv_idx = *(int*)data;
				}
				++recv_idx;
			}
		}));
	}

	std::thread producer([&]{
		while (muggle_atomic_load(&consumer_ready, muggle_memory_order_relaxed) != consumer_num);

		for (int i = 0; i < cnt; ++i)
		{
			int ret = muggle_ringbuffer_write(&r, &arr[i]);
			EXPECT_EQ(ret, (int)MUGGLE_OK);
		}
		muggle_ringbuffer_write(&r, &end_flag);
	});

	producer.join();
	for (auto &consumer : consumers)
	{
		consumer.join();
	}

	muggle_ringbuffer_destroy(&r);
	free(arr);
}

void mul_producer_one_consumer(int producer_num, int producer_need_yield)
{
	muggle_ringbuffer_t r;
	muggle_atomic_int capacity = 1024 * 16;
	muggle_atomic_int cnt = 1024 * 100;
	int *arr = (int*)malloc(sizeof(int) * cnt);
	int end_flag = -1;
	for (int i = 0; i < cnt; ++i)
	{
		arr[i] = i;
	}
	muggle_atomic_int consumer_ready = 0;

	int flag = MUGGLE_RINGBUFFER_FLAG_SINGLE_READER;
	if (producer_need_yield)
	{
		flag |= MUGGLE_RINGBUFFER_FLAG_WNUM_GT_CPU;
	}
	muggle_ringbuffer_init(&r, capacity, flag);

	std::thread consumer([&]{
		muggle_atomic_store(&consumer_ready, 1, muggle_memory_order_relaxed);

		muggle_atomic_int pos = 0;
		int recv_idx = 0;

		while (1)
		{
			void *data = muggle_ringbuffer_read(&r, pos++);
			if (data == &end_flag)
			{
				break;
			}
			// can't expect data >= recv_idx, cause can't control producer push order
			// EXPECT_GE(*(int*)data, recv_idx);
			if (*(int*)data != recv_idx)
			{
				// message loss
				recv_idx = *(int*)data;
			}
			++recv_idx;
		}
	});

	std::vector<std::thread> producers;
	muggle_atomic_int fetch_id = 0;
	for (int i = 0; i < producer_num; ++i)
	{
		producers.push_back(std::thread([&]{
			while (muggle_atomic_load(&consumer_ready, muggle_memory_order_relaxed) != 1);

			muggle_atomic_int idx;
			while (1)
			{
				idx = muggle_atomic_fetch_add(&fetch_id, 1, muggle_memory_order_relaxed);
				if (idx == cnt - 1)
				{
					muggle_ringbuffer_write(&r, &end_flag);
					break;
				}

				if (idx < cnt)
				{
					muggle_ringbuffer_write(&r, &arr[idx]);
				}
				else
				{
					break;
				}
			}
		}));
	}

	for (auto &producer : producers)
	{
		producer.join();
	}
	consumer.join();

	muggle_ringbuffer_destroy(&r);
	free(arr);
}

void mul_producer_mul_consumer(int producer_num, int producer_need_yield)
{
	muggle_ringbuffer_t r;
	muggle_atomic_int capacity = 1024 * 16;
	muggle_atomic_int cnt = 1024 * 100;
	int *arr = (int*)malloc(sizeof(int) * cnt);
	int end_flag = -1;
	int consumer_num = 5;
	for (int i = 0; i < cnt; ++i)
	{
		arr[i] = i;
	}
	muggle_atomic_int consumer_ready = 0;

	int flag = 0;
	if (producer_need_yield)
	{
		flag |= MUGGLE_RINGBUFFER_FLAG_WNUM_GT_CPU;
	}
	muggle_ringbuffer_init(&r, capacity, flag);

	std::vector<std::thread> consumers;
	for (int i = 0; i < consumer_num; ++i)
	{
		consumers.push_back(std::thread([&]{
			muggle_atomic_fetch_add(&consumer_ready, 1, muggle_memory_order_relaxed);

			muggle_atomic_int pos = 0;
			int recv_idx = 0;

			while (1)
			{
				void *data = muggle_ringbuffer_read(&r, pos++);
				if (data == &end_flag)
				{
					break;
				}
				// can't expect data >= recv_idx, cause can't control producer push order
				// EXPECT_GE(*(int*)data, recv_idx);
				if (*(int*)data != recv_idx)
				{
					// message loss
					recv_idx = *(int*)data;
				}
				++recv_idx;
			}
		}));
	}

	std::vector<std::thread> producers;
	muggle_atomic_int fetch_id = 0;
	for (int i = 0; i < producer_num; ++i)
	{
		producers.push_back(std::thread([&]{
			while (muggle_atomic_load(&consumer_ready, muggle_memory_order_acquire) != consumer_num);

			muggle_atomic_int idx;
			while (1)
			{
				idx = muggle_atomic_fetch_add(&fetch_id, 1, muggle_memory_order_relaxed);
				if (idx == cnt - 1)
				{
					muggle_ringbuffer_write(&r, &end_flag);
					break;
				}

				if (idx < cnt)
				{
					muggle_ringbuffer_write(&r, &arr[idx]);
				}
				else
				{
					break;
				}
			}
		}));
	}

	for (auto &producer : producers)
	{
		producer.join();
	}
	for (auto &consumer : consumers)
	{
		consumer.join();
	}

	muggle_ringbuffer_destroy(&r);
	free(arr);
}

TEST(ringbuffer, mul_producer_one_consumer)
{
	auto writer_num = std::thread::hardware_concurrency();
	if (writer_num == 0)
	{
		writer_num = 2;
	}
	mul_producer_one_consumer(writer_num, 0);
}

TEST(ringbuffer, mul_producer_gt_cpu_one_consumer)
{
	auto writer_num = std::thread::hardware_concurrency();
	if (writer_num == 0)
	{
		writer_num = 16;
	}
	mul_producer_one_consumer(writer_num * 2, 1);
}

TEST(ringbuffer, mul_producer_mul_consumer)
{
	auto writer_num = std::thread::hardware_concurrency();
	if (writer_num == 0)
	{
		writer_num = 2;
	}
	mul_producer_mul_consumer(writer_num, 0);
}

TEST(ringbuffer, mul_producer_gt_cpu_mul_consumer)
{
	auto writer_num = std::thread::hardware_concurrency();
	if (writer_num == 0)
	{
		writer_num = 16;
	}
	mul_producer_mul_consumer(writer_num * 2, 1);
}