contrib/gmock/gmock-gtest-all.o:
	g++ -c contrib/gmock/gmock-gtest-all.cc -I./contrib/gmock/ -o contrib/gmock/gmock-gtest-all.o -fPIC

tmps += contrib/gmock/gmock-gtest-all.o

lib/libpd-gtest.s.a: contrib/gmock/gmock-gtest-all.o
	ar crs lib/libpd-gtest.s.a contrib/gmock/gmock-gtest-all.o

targets += lib/libpd-gtest.s.a
