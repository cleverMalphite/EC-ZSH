#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
//get time string
#include "../../ec2/public/_public.h"
#include "./infoHub.h"

using namespace std;

infoHub info_hub;

int main(void)
{

  printf("========= KV TEST ========\n");
  //test kv int test
  info_hub.kv_set_int("epollcomm", "tx_rate1", 1);
  info_hub.kv_set_int("epollcomm", "tx_rate2", 2);
  info_hub.kv_set_int("epollcomm", "tx_rate3", 3);
  info_hub.kv_set_int("epollcomm", "tx_rate4", 4);
  info_hub.kv_set_int("epollcomm", "tx_rate5", 5);
  info_hub.kv_set_int("epollcomm", "tx_rate6", 6);

  int epoll_rate=info_hub.kv_get_int("epollcomm", "tx_rate2");
  printf(">>test1 kv int test\n");
  printf("epollcomm tx_rate=%d\n", epoll_rate);

  info_hub.kv_del_int("epollcomm", "tx_rate1");
  info_hub.kv_del_int("epollcomm", "tx_rate2");
  printf("\n");

  //test kv string test
  info_hub.kv_set_str("epollcomm", "file1", "/a/b/c/d/file1.txt");
  info_hub.kv_set_str("epollcomm", "file2", "/a/b/c/d/file2.txt");
  info_hub.kv_set_str("epollcomm", "file3", "/a/b/c/d/file3.txt");
  info_hub.kv_set_str("epollcomm", "file4", "/a/b/c/d/file4.txt");
  info_hub.kv_set_str("epollcomm", "file5", "/a/b/c/d/file5.txt");
  info_hub.kv_set_str("epollcomm", "file6", "/a/b/c/d/file6.txt");

  string epoll_file=info_hub.kv_get_str("epollcomm", "file2");
  printf(">>test2 kv str test\n");
  printf("epollcomm file=%s\n", epoll_file.c_str());

  info_hub.kv_del_str("epollcomm", "file2");
  printf("\n");


  printf("========= KM TEST ========\n");
  //test km int test
  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate1", /*tid*/1, /*rate*/10);
  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate1", /*tid*/2, /*rate*/20);
  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate1", /*tid*/3, /*rate*/30);
  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate1", /*tid*/4, /*rate*/40);


  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate2", /*tid*/1, /*rate*/11);
  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate2", /*tid*/2, /*rate*/22);
  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate2", /*tid*/3, /*rate*/33);
  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate2", /*tid*/4, /*rate*/44);

  int n_t1_1=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate1", 1);
  int n_t1_2=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate1", 2);
  int n_t1_3=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate1", 2);
  int n_t1_4=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate1", 3);

  int n_t2_1=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate2", 1);
  int n_t2_2=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate2", 2);
  int n_t2_3=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate2", 3);
  int n_t2_4=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate2", 4);

  printf(">>test3 km int test\n");
  printf("netcombtransfer n_t1_1=%d\n", n_t1_1);
  printf("netcombtransfer n_t1_2=%d\n", n_t1_2);
  printf("netcombtransfer n_t1_3=%d\n", n_t1_3);
  printf("netcombtransfer n_t1_4=%d\n", n_t1_4);

  printf("netcombtransfer n_t2_1=%d\n", n_t2_1);
  printf("netcombtransfer n_t2_2=%d\n", n_t2_2);
  printf("netcombtransfer n_t2_3=%d\n", n_t2_3);
  printf("netcombtransfer n_t2_4=%d\n", n_t2_4);

  info_hub.km_del_i2i("netcombtransfer", "tx_tid_rate1", 1);
  info_hub.km_del_i2i("netcombtransfer", "tx_tid_rate1", 2);
  info_hub.km_del_i2i("netcombtransfer", "tx_tid_rate1", 3);
  info_hub.km_del_i2i("netcombtransfer", "tx_tid_rate1", 4);

  info_hub.km_del_i2i("netcombtransfer", "tx_tid_rate2", 1);
  info_hub.km_del_i2i("netcombtransfer", "tx_tid_rate2", 2);
  info_hub.km_del_i2i("netcombtransfer", "tx_tid_rate2", 3);
  info_hub.km_del_i2i("netcombtransfer", "tx_tid_rate2", 4);

  printf("\n");


  //test km str test
  info_hub.km_set_i2s("netcombtransfer", "send_task_dir", /*taskid*/1, /*dir*/"/home/a/b/c/file1.txt");
  info_hub.km_set_i2s("netcombtransfer", "send_task_dir", /*taskid*/2, /*dir*/"/home/a/b/c/file2.txt");
  info_hub.km_set_i2s("netcombtransfer", "send_task_dir", /*taskid*/3, /*dir*/"/home/a/b/c/file3.txt");
  info_hub.km_set_i2s("netcombtransfer", "send_task_dir", /*taskid*/4, /*dir*/"/home/a/b/c/file4.txt");


  info_hub.km_set_i2s("netcombtransfer", "recv_task_dir", /*taskid*/1, /*dir*/"/home/a/b/c/file11.txt");
  info_hub.km_set_i2s("netcombtransfer", "recv_task_dir", /*taskid*/2, /*dir*/"/home/a/b/c/file22.txt");
  info_hub.km_set_i2s("netcombtransfer", "recv_task_dir", /*taskid*/3, /*dir*/"/home/a/b/c/file33.txt");
  info_hub.km_set_i2s("netcombtransfer", "recv_task_dir", /*taskid*/4, /*dir*/"/home/a/b/c/file44.txt");

  std::string n_r_1=info_hub.km_get_i2s("netcombtransfer", "send_task_dir", 1);
  std::string n_r_2=info_hub.km_get_i2s("netcombtransfer", "send_task_dir", 2);
  std::string n_r_3=info_hub.km_get_i2s("netcombtransfer", "send_task_dir", 3);
  std::string n_r_4=info_hub.km_get_i2s("netcombtransfer", "send_task_dir", 4);

  std::string n_s_1=info_hub.km_get_i2s("netcombtransfer", "recv_task_dir", 1);
  std::string n_s_2=info_hub.km_get_i2s("netcombtransfer", "recv_task_dir", 2);
  std::string n_s_3=info_hub.km_get_i2s("netcombtransfer", "recv_task_dir", 3);
  std::string n_s_4=info_hub.km_get_i2s("netcombtransfer", "recv_task_dir", 4);

  printf(">>test4 km str test\n");
  printf("netcombtransfer n_r_1=%s\n", n_r_1.c_str());
  printf("netcombtransfer n_r_2=%s\n", n_r_2.c_str());
  printf("netcombtransfer n_r_3=%s\n", n_r_3.c_str());
  printf("netcombtransfer n_r_4=%s\n", n_r_4.c_str());
                                  
  printf("netcombtransfer n_s_1=%s\n", n_s_1.c_str());
  printf("netcombtransfer n_s_2=%s\n", n_s_2.c_str());
  printf("netcombtransfer n_s_3=%s\n", n_s_3.c_str());
  printf("netcombtransfer n_s_4=%s\n", n_s_4.c_str());

  info_hub.km_del_i2s("netcombtransfer", "send_task_dir", 1);
  info_hub.km_del_i2s("netcombtransfer", "send_task_dir", 2);
  info_hub.km_del_i2s("netcombtransfer", "send_task_dir", 3);
  info_hub.km_del_i2s("netcombtransfer", "send_task_dir", 4);
                    
  info_hub.km_del_i2s("netcombtransfer", "recv_task_dir", 1);
  info_hub.km_del_i2s("netcombtransfer", "recv_task_dir", 2);
  info_hub.km_del_i2s("netcombtransfer", "recv_task_dir", 3);
  info_hub.km_del_i2s("netcombtransfer", "recv_task_dir", 4);

  printf("\n");

  printf("========= KS TEST ========\n");
  //test ks int test
  info_hub.ks_add_int("bigdatatransfer", "connected_tid", /*tid*/1);
  info_hub.ks_add_int("bigdatatransfer", "connected_tid", /*tid*/2);
  info_hub.ks_add_int("bigdatatransfer", "connected_tid", /*tid*/3);

  bool b_c_1=info_hub.ks_ism_int("bigdatatransfer", "connected_tid", 1);
  bool b_c_2=info_hub.ks_ism_int("bigdatatransfer", "connected_tid", 2);
  bool b_c_3=info_hub.ks_ism_int("bigdatatransfer", "connected_tid", 3);
  bool b_c_4=info_hub.ks_ism_int("bigdatatransfer", "connected_tid", 4);

  printf(">>test5 km int test\n");
  printf("bigdatatransfer has b_c_1=%d(true/false)\n", b_c_1);
  printf("bigdatatransfer has b_c_2=%d(true/false)\n", b_c_2);
  printf("bigdatatransfer has b_c_3=%d(true/false)\n", b_c_3);
  printf("bigdatatransfer has b_c_4=%d(true/false)\n", b_c_4);

  info_hub.ks_del_int("bigdatatransfer", "connected_tid", 1);
  info_hub.ks_del_int("bigdatatransfer", "connected_tid", 2);
  info_hub.ks_del_int("bigdatatransfer", "connected_tid", 3);
  //info_hub.ks_del_int("bigdatatransfer", "connected_tid", 4);

  printf("\n");

  //test ks int test
  info_hub.ks_add_str("bigdatatransfer", "files", /*file*/"a/b/c/d/1.txt");
  info_hub.ks_add_str("bigdatatransfer", "files", /*file*/"a/b/c/d/2.txt");
  info_hub.ks_add_str("bigdatatransfer", "files", /*file*/"a/b/c/d/3.txt");

  bool b_f_1=info_hub.ks_ism_str("bigdatatransfer", "files", "a/b/c/d/1.txt");
  bool b_f_2=info_hub.ks_ism_str("bigdatatransfer", "files", "a/b/c/d/2.txt");
  bool b_f_3=info_hub.ks_ism_str("bigdatatransfer", "files", "a/b/c/d/3.txt");
  bool b_f_4=info_hub.ks_ism_str("bigdatatransfer", "files", "a/b/c/d/4.txt");

  printf(">>test6 km int test\n");
  printf("bigdatatransfer has b_c_1=%d(true/false)\n", b_f_1);
  printf("bigdatatransfer has b_c_2=%d(true/false)\n", b_f_2);
  printf("bigdatatransfer has b_c_3=%d(true/false)\n", b_f_3);
  printf("bigdatatransfer has b_c_4=%d(true/false)\n", b_f_4);

  info_hub.ks_del_str("bigdatatransfer", "files", /*file*/"a/b/c/d/1.txt");
  info_hub.ks_del_str("bigdatatransfer", "files", /*file*/"a/b/c/d/2.txt");
  info_hub.ks_del_str("bigdatatransfer", "files", /*file*/"a/b/c/d/3.txt");
  //info_hub.ks_del_str("bigdatatransfer", "files", /*file*/"a/b/c/d/4.txt");

  printf("\n");

  printf("========= empty Get TEST========\n");
{
  //the table below has been delete, so test the empty read wrong
  int epoll_rate=info_hub.kv_get_int("epollcomm", "tx_rate2");
  printf(">>kv int test\n");
  printf("epollcomm tx_rate=%d\n", epoll_rate);
  printf("++++++++++++++\n");

  string epoll_file=info_hub.kv_get_str("epollcomm", "file2");
  printf(">>kv str test\n");
  printf("epollcomm file=%s\n", epoll_file.c_str());
  printf("++++++++++++++\n");

  int n_t1_1=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate1", 1);
  printf(">>km int test\n");
  printf("netcombtransfer n_t1_1=%d\n", n_t1_1);
  printf("++++++++++++++\n");

  std::string n_r_1=info_hub.km_get_i2s("netcombtransfer", "send_task_dir", 1);
  printf(">>km str test\n");
  printf("netcombtransfer n_r_1=%s\n", n_r_1.c_str());
  printf("++++++++++++++\n");

  bool b_c_1=info_hub.ks_ism_int("bigdatatransfer", "connected_tid", 1);
  printf(">>ks int test\n");
  printf("bigdatatransfer has b_c_1=%d(true/false)\n", b_c_1);
  printf("++++++++++++++\n");
  
  bool b_f_1=info_hub.ks_ism_str("bigdatatransfer", "files", "a/b/c/d/1.txt");
  printf(">>ks str test\n");
  printf("bigdatatransfer has b_f_1=%d(true/false)\n", b_f_1);
  printf("++++++++++++++\n");

  printf("\n");
}

  printf("========= DOUBLE TEST ========\n");

{
  printf("========= KV \n");
  //test kv int test
  info_hub.kv_set_int("epollcomm", "tx_rate1", 1);
  info_hub.kv_set_int("epollcomm", "tx_rate1", 101);
  int epoll_rate=info_hub.kv_get_int("epollcomm", "tx_rate1");
  printf(">>test1 kv int test\n");
  printf("epollcomm tx_rate=%d\n", epoll_rate);
  info_hub.kv_del_int("epollcomm", "tx_rate1");

  //test kv string test
  info_hub.kv_set_str("epollcomm", "file1", "/a/b/c/d/file1.txt");
  info_hub.kv_set_str("epollcomm", "file1", "/a/b/c/d/file101.txt");
  string epoll_file=info_hub.kv_get_str("epollcomm", "file1");
  printf(">>test2 kv str test\n");
  printf("epollcomm file=%s\n", epoll_file.c_str());
  info_hub.kv_del_str("epollcomm", "file1");

  printf("========= KM \n");
  //test km int test
  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate1", /*tid*/1, /*rate*/1);
  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate1", /*tid*/1, /*rate*/101);
  int n_t1_1=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate1", 1);
  printf(">>test3 km int test\n");
  printf("netcombtransfer n_t1_1=%d\n", n_t1_1);
  info_hub.km_del_i2i("netcombtransfer", "tx_tid_rate1", 1);

  //test km str test
  info_hub.km_set_i2s("netcombtransfer", "send_task_dir", /*taskid*/1, /*dir*/"/home/a/b/c/file1.txt");
  info_hub.km_set_i2s("netcombtransfer", "send_task_dir", /*taskid*/1, /*dir*/"/home/a/b/c/file101.txt");
  std::string n_r_1=info_hub.km_get_i2s("netcombtransfer", "send_task_dir", 1);
  printf(">>test4 km str test\n");
  printf("netcombtransfer n_r_1=%s\n", n_r_1.c_str());
  info_hub.km_del_i2s("netcombtransfer", "send_task_dir", 1);

  printf("========= KS \n");
  //test ks int test
  info_hub.ks_add_int("bigdatatransfer", "connected_tid", /*tid*/1);
  info_hub.ks_add_int("bigdatatransfer", "connected_tid", /*tid*/1);
  bool b_c_1=info_hub.ks_ism_int("bigdatatransfer", "connected_tid", 1);
  printf(">>test5 km int test\n");
  printf("bigdatatransfer has b_c_1=%d(true/false)\n", b_c_1);
  info_hub.ks_del_int("bigdatatransfer", "connected_tid", 1);

  //test ks int test
  info_hub.ks_add_str("bigdatatransfer", "files", /*file*/"a/b/c/d/1.txt");
  info_hub.ks_add_str("bigdatatransfer", "files", /*file*/"a/b/c/d/1.txt");
  bool b_f_1=info_hub.ks_ism_str("bigdatatransfer", "files", "a/b/c/d/1.txt");
  printf(">>test6 km int test\n");
  printf("bigdatatransfer has b_c_1=%d(true/false)\n", b_f_1);
  info_hub.ks_del_str("bigdatatransfer", "files", /*file*/"a/b/c/d/1.txt");

  printf("\n");
}



  printf("========= WRONG TYPE TEST ========\n");
  //exist sample
  //info_hub.kv_set_int("epollcomm", "tx_rate1", 1);
  //info_hub.kv_set_str("epollcomm", "file1", "/a/b/c/d/file1.txt");

  //info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate1", /*tid*/1, /*rate*/10);
  //info_hub.km_set_i2s("netcombtransfer", "send_task_dir", /*taskid*/2, /*dir*/"/home/a/b/c/file2.txt");

  //info_hub.ks_add_int("bigdatatransfer", "connected_tid", /*tid*/1);
  //info_hub.ks_add_str("bigdatatransfer", "files", /*file*/"a/b/c/d/1.txt");

  //new set but wrong type
  info_hub.kv_set_int("epollcomm", "file1", 1);
  info_hub.kv_set_str("epollcomm", "tx_rate1", "/a/b/c/d/file1.txt");

  info_hub.km_set_i2i("netcombtransfer", "send_task_dir", /*tid*/1, /*rate*/10);
  info_hub.km_set_i2s("netcombtransfer", "tx_tid_rate1", /*taskid*/2, /*dir*/"/home/a/b/c/file2.txt");

  info_hub.ks_add_int("bigdatatransfer", "files", /*tid*/1);
  info_hub.ks_add_str("bigdatatransfer", "connected_tid", /*file*/"a/b/c/d/1.txt");

  return 0;
}
