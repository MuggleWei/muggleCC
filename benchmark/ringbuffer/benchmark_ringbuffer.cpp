/*
 *	author: muggle wei <mugglewei@gmail.com>
 *
 *	Use of this source code is governed by the MIT license that can be
 *	found in the LICENSE file.
 */

#include "muggle_benchmark/muggle_benchmark.h"
#include <stdio.h>
#include <thread>
#include <chrono>

void fn_producer(
	muggle_ringbuffer_t *ring,
	muggle::BenchmarkConfig *config,
	muggle::LatencyBlock *blocks,
	muggle_atomic_int *consumer_ready,
	muggle_atomic_int consumer_cnt,
	uint64_t start_idx,
	uint64_t end_idx
)
{
	while (muggle_atomic_load(consumer_ready, muggle_memory_order_relaxed) != consumer_cnt);

	for (uint64_t i = 0; i < config->loop; ++i)
	{
		for (uint64_t j = start_idx; j < end_idx; ++j)
		{
			uint64_t idx = i * config->cnt_per_loop + j;
			memset(&blocks[idx], 0, sizeof(muggle::LatencyBlock));
			blocks[idx].idx = idx;

			timespec_get(&blocks[idx].ts[0], TIME_UTC);
			muggle_ringbuffer_push(ring, &blocks[idx], 1, 1);
			timespec_get(&blocks[idx].ts[1], TIME_UTC);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(config->loop_interval_ms));
	}
}

void fn_consumer(
	muggle_ringbuffer_t *ring,
	muggle_atomic_int *consumer_ready,
	int print_msg_loss
)
{
	muggle_atomic_fetch_add(consumer_ready, 1, muggle_memory_order_relaxed);
	muggle_atomic_int pos = 0;
	uint64_t idx = 0;
	while (1)
	{
		muggle::LatencyBlock *block = (muggle::LatencyBlock*)muggle_ringbuffer_get(ring, pos++);
		if (!block)
		{
			break;
		}
		timespec_get(&block->ts[2], TIME_UTC);
		if (print_msg_loss && idx != block->idx)
		{
			printf("expect: %llu, actual: %llu\n", (unsigned long long)idx, (unsigned long long)block->idx);
			idx = block->idx;
		}
		++idx;
	}
}

void Benchmark_wr(FILE *fp, muggle::BenchmarkConfig &config, int cnt_producer, int cnt_consumer)
{
	uint64_t cnt = config.loop * config.cnt_per_loop;
	muggle::LatencyBlock *blocks = (muggle::LatencyBlock*)malloc(cnt * sizeof(muggle::LatencyBlock));
	muggle_atomic_int consumer_ready = 0;

	muggle_ringbuffer_t ring;
	muggle_ringbuffer_init(&ring, 1024 * 16);

	std::vector<std::thread> producers;
	std::vector<std::thread> consumers;

	for (int i = 0; i < cnt_producer; ++i)
	{
		uint64_t start_idx = i * (config.cnt_per_loop / cnt_producer);
		uint64_t end_idx = (i + 1) * (config.cnt_per_loop / cnt_producer);
		if (i == cnt_producer - 1)
		{
			end_idx = config.cnt_per_loop;
		}

		producers.push_back(std::thread(fn_producer,
			&ring, &config, blocks, &consumer_ready, cnt_consumer, start_idx, end_idx
		));
	}
	for (int i = 0; i < cnt_consumer; ++i)
	{
		consumers.push_back(std::thread(fn_consumer,
			&ring, &consumer_ready, 0
		));
	}

	for (auto &producer : producers)
	{
		producer.join();
	}

	muggle_ringbuffer_push(&ring, nullptr, 1, 0);

	for (auto &consumer : consumers)
	{
		consumer.join();
	}

	muggle_ringbuffer_destroy(&ring);

	char buf[128];

	snprintf(buf, sizeof(buf) - 1, "%dw%dr-w", cnt_producer, cnt_consumer);
	muggle::GenLatencyReportsBody(fp, &config, blocks, buf, cnt, 0, 1, 0);

	snprintf(buf, sizeof(buf) - 1, "%dw%dr-w-sorted", cnt_producer, cnt_consumer);
	muggle::GenLatencyReportsBody(fp, &config, blocks, buf, cnt, 0, 1, 1);

	snprintf(buf, sizeof(buf) - 1, "%dw%dr-wr", cnt_producer, cnt_consumer);
	muggle::GenLatencyReportsBody(fp, &config, blocks, buf, cnt, 0, 2, 0);

	snprintf(buf, sizeof(buf) - 1, "%dw%dr-wr-sorted", cnt_producer, cnt_consumer);
	muggle::GenLatencyReportsBody(fp, &config, blocks, buf, cnt, 0, 2, 1);

	free(blocks);
}

int main()
{
	muggle::BenchmarkConfig config;
	strncpy(config.name, "ringbuffer", sizeof(config.name)-1);
	config.loop = 50;
	config.cnt_per_loop = 10000;
	config.loop_interval_ms = 1;
	config.report_step = 10;

	char file_name[128];
	snprintf(file_name, sizeof(file_name)-1, "benchmark_%s.csv", config.name);
	FILE *fp = fopen(file_name, "wb");
	if (fp == nullptr)
	{
		printf("failed open file: %s\n", file_name);
		exit(1);
	}

	muggle::GenLatencyReportsHead(fp, &config);

	// 1 writer, 1 reader
	Benchmark_wr(fp, config, 1, 1);

	// 2 writer, 1 reader
	Benchmark_wr(fp, config, 2, 1);

	// 8 writer, 1 reader
	Benchmark_wr(fp, config, 8, 1);

	// 32 writer, 1 reader
	Benchmark_wr(fp, config, 32, 1);

	fclose(fp);
}
