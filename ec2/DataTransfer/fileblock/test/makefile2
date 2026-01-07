all:\
	test_main\
	test_main2\
	test_main3_random_order\
	test_main4_random_order\

cppflags=-g -pthread
cppfile=../../../Util/SystemTimeFunc.cpp ../msg_cpp/FileBlockMsg.pb.cc
headfile=../WriteFileBlock.h ../ReadFileBlock.h ../msg_cpp/FileBlockMsg.pb.h

lib=-lprotobuf -lpthread

INCLUDE=-I../. -I../msg_cpp

test_main:test_main.cpp ${cppfile} ${headfile}
	g++ ${cppflags} -o test_main test_main.cpp ${cppfile} ${INCLUDE} ${lib}

test_main2:test_main2.cpp ${cppfile}  ${headfile}
	g++ ${cppflags} -o test_main2 test_main2.cpp  ${cppfile} ${INCLUDE} ${lib}

test_main3_random_order:test_main3_random_order.cpp  ${cppfile} ${headfile} 
	g++ ${cppflags} -o test_main3_random_order test_main3_random_order.cpp  ${cppfile} ${INCLUDE} ${lib}

test_main4_random_order:test_main4_random_order.cpp  ${cppfile} ${headfile}
	g++ ${cppflags} -o test_main4_random_order test_main4_random_order.cpp   ${cppfile} ${INCLUDE} ${lib}

clean:
	rm -f test_main test_main2 test_main3_random_order test_main4_random_order
