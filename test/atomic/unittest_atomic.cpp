#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "gtest/gtest.h"
#include "muggle/c/muggle_c.h"

TEST(atomic, load_store_single_thread)
{
	muggle_atomic_int v;
	muggle_atomic_store(&v, 0, muggle_memory_order_relaxed);
	EXPECT_EQ(muggle_atomic_load(&v, muggle_memory_order_relaxed), 0);
}

TEST(atomic, load_store_inter_thread_visible_busy_loop)
{
	muggle_atomic_int v = 0;

	std::thread t([&]{
		muggle_atomic_store(&v, 1, muggle_memory_order_relaxed);
	});
	while (muggle_atomic_load(&v, muggle_memory_order_relaxed) != 1);
	t.join();
	EXPECT_EQ(muggle_atomic_load(&v, muggle_memory_order_relaxed), 1);
}

TEST(atomic, load_store_inter_thread_visible_mutex)
{
	muggle_atomic_int v = 0;

	std::mutex mtx;
	std::unique_lock<std::mutex> lock_main(mtx);

	std::thread t([&v, &mtx]{
		std::unique_lock<std::mutex> lock(mtx);
		EXPECT_EQ(muggle_atomic_load(&v, muggle_memory_order_relaxed), 1);
	});

	muggle_atomic_store(&v, 1, muggle_memory_order_relaxed);
	lock_main.unlock();

	t.join();
}

TEST(atomic, load_store_inter_thread_visible_condition_variable)
{
	muggle_atomic_int v = 0;

	std::mutex mtx;
	std::condition_variable cv;

	std::unique_lock<std::mutex> lock_main(mtx);

	std::thread t([&v, &mtx, &cv]{
		std::unique_lock<std::mutex> lock(mtx);
		muggle_atomic_store(&v, 1, muggle_memory_order_relaxed);
		cv.notify_one();
	});
	t.detach();

	cv.wait(lock_main);
	EXPECT_EQ(muggle_atomic_load(&v, muggle_memory_order_relaxed), 1);
}

TEST(atomic, load_store_release_acquire)
{
	muggle_atomic_int x, y;
	x = 0;
	y = 0;

	std::thread write_x_then_y([&]{
		muggle_atomic_store(&x, 1, muggle_memory_order_relaxed);
		muggle_atomic_store(&y, 1, muggle_memory_order_release);
	});
	std::thread read_y_then_x([&]{
		while (muggle_atomic_load(&y, muggle_memory_order_acquire) != 1);
		EXPECT_EQ(muggle_atomic_load(&x, muggle_memory_order_relaxed), 1);
	});
	write_x_then_y.join();
	read_y_then_x.join();
}

TEST(atomic, exchange_single_thread)
{
	muggle_atomic_int i = 0;
	muggle_atomic_int32 i32 = 0;
	muggle_atomic_int64 i64 = 0;

	EXPECT_EQ(muggle_atomic_exchange(&i, 1, muggle_memory_order_relaxed), 0);
	EXPECT_EQ(muggle_atomic_exchange32(&i32, 1, muggle_memory_order_relaxed), 0);
	EXPECT_EQ(muggle_atomic_exchange64(&i64, 1, muggle_memory_order_relaxed), 0);

	EXPECT_EQ(i, 1);
	EXPECT_EQ(i32, 1);
	EXPECT_EQ(i64, 1);
}

TEST(atomic, cmp_exch_single_thread)
{
	muggle_atomic_int i = 0, expected = 1;
	muggle_atomic_int32 i32 = 0, expected32 = 1;
	muggle_atomic_int64 i64 = 0, expected64 = 1;

	EXPECT_FALSE(muggle_atomic_cmp_exch_weak(&i, &expected, 2, muggle_memory_order_relaxed));
	EXPECT_EQ(expected, 0);

	EXPECT_FALSE(muggle_atomic_cmp_exch_weak32(&i32, &expected32, 2, muggle_memory_order_relaxed));
	EXPECT_EQ(expected32, 0);

	EXPECT_FALSE(muggle_atomic_cmp_exch_weak64(&i64, &expected64, 2, muggle_memory_order_relaxed));
	EXPECT_EQ(expected64, 0);

	i = 0;
	expected = 0;
	while (!muggle_atomic_cmp_exch_weak(&i, &expected, 1, muggle_memory_order_relaxed) && expected == 0);
	EXPECT_EQ(expected, 0);
	EXPECT_EQ(i, 1);

	i32 = 0;
	expected32 = 0;
	while (!muggle_atomic_cmp_exch_weak32(&i32, &expected32, 1, muggle_memory_order_relaxed) && expected32 == 0);
	EXPECT_EQ(expected32, 0);
	EXPECT_EQ(i32, 1);

	i64 = 0;
	expected64 = 0;
	while (!muggle_atomic_cmp_exch_weak64(&i64, &expected64, 1, muggle_memory_order_relaxed) && expected64 == 0);
	EXPECT_EQ(expected64, 0);
	EXPECT_EQ(i64, 1);

	i = 0;
	expected = 0;
	EXPECT_TRUE(muggle_atomic_cmp_exch_strong(&i, &expected, 1, muggle_memory_order_relaxed));
	EXPECT_EQ(expected, 0);
	EXPECT_EQ(i, 1);

	i32 = 0;
	expected32 = 0;
	EXPECT_TRUE(muggle_atomic_cmp_exch_strong32(&i32, &expected32, 1, muggle_memory_order_relaxed));
	EXPECT_EQ(expected32, 0);
	EXPECT_EQ(i32, 1);

	i64 = 0;
	expected64 = 0;
	EXPECT_TRUE(muggle_atomic_cmp_exch_strong64(&i64, &expected64, 1, muggle_memory_order_relaxed));
	EXPECT_EQ(expected64, 0);
	EXPECT_EQ(i64, 1);
}

TEST(atomic, fetch_add_sub_single_thread)
{
	muggle_atomic_int i = 0;
	muggle_atomic_int32 i32 = 0;
	muggle_atomic_int64 i64 = 0;

	EXPECT_EQ(muggle_atomic_fetch_add(&i, 1, muggle_memory_order_relaxed), 0);
	EXPECT_EQ(i, 1);
	EXPECT_EQ(muggle_atomic_fetch_sub(&i, 1, muggle_memory_order_relaxed), 1);
	EXPECT_EQ(i, 0);

	EXPECT_EQ(muggle_atomic_fetch_add32(&i32, 1, muggle_memory_order_relaxed), 0);
	EXPECT_EQ(i32, 1);
	EXPECT_EQ(muggle_atomic_fetch_sub32(&i32, 1, muggle_memory_order_relaxed), 1);
	EXPECT_EQ(i32, 0);

	EXPECT_EQ(muggle_atomic_fetch_add64(&i64, 1, muggle_memory_order_relaxed), 0);
	EXPECT_EQ(i64, 1);
	EXPECT_EQ(muggle_atomic_fetch_sub64(&i64, 1, muggle_memory_order_relaxed), 1);
	EXPECT_EQ(i64, 0);
}

TEST(atomic, fetch_add_release_acquire)
{
	muggle_atomic_int x, y, z;
	x = 0;
	y = 1;
	z = 0;

	std::thread t1([&]{
		muggle_atomic_fetch_add(&x, 1, muggle_memory_order_relaxed);
		muggle_atomic_fetch_sub(&y, 1, muggle_memory_order_relaxed);
		muggle_atomic_store(&z, 1, muggle_memory_order_release);
	});
	std::thread t2([&]{
		while (muggle_atomic_load(&z, muggle_memory_order_acquire) != 1);
		EXPECT_EQ(muggle_atomic_load(&x, muggle_memory_order_relaxed), 1);
		EXPECT_EQ(muggle_atomic_load(&y, muggle_memory_order_relaxed), 0);
	});
	t1.join();
	t2.join();
}

TEST(atomic, test_set_clear)
{
	muggle_atomic_byte b;

	muggle_atomic_clear(&b, muggle_memory_order_relaxed);
	EXPECT_TRUE(muggle_atomic_test_and_set(&b, muggle_memory_order_relaxed));
	EXPECT_FALSE(muggle_atomic_test_and_set(&b, muggle_memory_order_relaxed));
	muggle_atomic_clear(&b, muggle_memory_order_relaxed);
	EXPECT_TRUE(muggle_atomic_test_and_set(&b, muggle_memory_order_relaxed));
}

TEST(atomic, fence)
{
	muggle_atomic_int x, y;
	x = 0;
	y = 0;

	std::thread write_x_then_y([&]{
		muggle_atomic_store(&x, 1, muggle_memory_order_relaxed);
		muggle_atomic_store(&y, 1, muggle_memory_order_relaxed);
		muggle_atomic_thread_fence(muggle_memory_order_release);
	});
	std::thread read_y_then_x([&]{
		muggle_atomic_thread_fence(muggle_memory_order_acquire);
		while (muggle_atomic_load(&y, muggle_memory_order_relaxed) != 1);
		EXPECT_EQ(muggle_atomic_load(&x, muggle_memory_order_relaxed), 1);
	});
	write_x_then_y.join();
	read_y_then_x.join();
}
