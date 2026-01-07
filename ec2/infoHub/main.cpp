#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
//get time string
#include "../../ec2/public/_public.h"
#include "./infoHub_impl.h"

using namespace std;


infoHub_impl info_hub;

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

  //test kv int test
  info_hub.kv_set_lf("epollcomm", "tx_ratelf1", 1.1);
  info_hub.kv_set_lf("epollcomm", "tx_ratelf2", 2.1);
  info_hub.kv_set_lf("epollcomm", "tx_ratelf3", 3.1);
  info_hub.kv_set_lf("epollcomm", "tx_ratelf4", 4.1);
  info_hub.kv_set_lf("epollcomm", "tx_ratelf5", 5.1);
  info_hub.kv_set_lf("epollcomm", "tx_ratelf6", 6.1);

  double epoll_ratelf1=info_hub.kv_get_lf("epollcomm", "tx_ratelf1");
  double epoll_ratelf2=info_hub.kv_get_lf("epollcomm", "tx_ratelf2");
  double epoll_ratelf3=info_hub.kv_get_lf("epollcomm", "tx_ratelf3");
  double epoll_ratelf4=info_hub.kv_get_lf("epollcomm", "tx_ratelf4");
  double epoll_ratelf5=info_hub.kv_get_lf("epollcomm", "tx_ratelf5");
  double epoll_ratelf6=info_hub.kv_get_lf("epollcomm", "tx_ratelf6");
  printf(">>test1 kv lf test\n");
  printf("epollcomm tx_ratelf1=%lf\n", epoll_ratelf1);
  printf("epollcomm tx_ratelf2=%lf\n", epoll_ratelf2);
  printf("epollcomm tx_ratelf3=%lf\n", epoll_ratelf3);
  printf("epollcomm tx_ratelf4=%lf\n", epoll_ratelf4);
  printf("epollcomm tx_ratelf5=%lf\n", epoll_ratelf5);
  printf("epollcomm tx_ratelf6=%lf\n", epoll_ratelf6);

  info_hub.kv_del_lf("epollcomm", "tx_ratelf1");
  info_hub.kv_del_lf("epollcomm", "tx_ratelf2");
  info_hub.kv_del_lf("epollcomm", "tx_ratelf3");
  info_hub.kv_del_lf("epollcomm", "tx_ratelf4");
  info_hub.kv_del_lf("epollcomm", "tx_ratelf5");
  info_hub.kv_del_lf("epollcomm", "tx_ratelf6");
  printf("\n");

  //test kv int test
  info_hub.kv_set_u32("epollcomm", "tx_rateu32_1", 1);
  info_hub.kv_set_u32("epollcomm", "tx_rateu32_2", 2);
  info_hub.kv_set_u32("epollcomm", "tx_rateu32_3", 3);
  info_hub.kv_set_u32("epollcomm", "tx_rateu32_4", 4);
  info_hub.kv_set_u32("epollcomm", "tx_rateu32_5", 5);
  info_hub.kv_set_u32("epollcomm", "tx_rateu32_6", 6);

  uint32_t epoll_rateu32_1=info_hub.kv_get_u32("epollcomm", "tx_rateu32_1");
  uint32_t epoll_rateu32_2=info_hub.kv_get_u32("epollcomm", "tx_rateu32_2");
  uint32_t epoll_rateu32_3=info_hub.kv_get_u32("epollcomm", "tx_rateu32_3");
  uint32_t epoll_rateu32_4=info_hub.kv_get_u32("epollcomm", "tx_rateu32_4");
  uint32_t epoll_rateu32_5=info_hub.kv_get_u32("epollcomm", "tx_rateu32_5");
  uint32_t epoll_rateu32_6=info_hub.kv_get_u32("epollcomm", "tx_rateu32_6");
  printf(">>test1 kv uint32_t test\n");
  printf("epollcomm tx_rateu32_1=%u\n", epoll_rateu32_1);
  printf("epollcomm tx_rateu32_2=%u\n", epoll_rateu32_2);
  printf("epollcomm tx_rateu32_3=%u\n", epoll_rateu32_3);
  printf("epollcomm tx_rateu32_4=%u\n", epoll_rateu32_4);
  printf("epollcomm tx_rateu32_5=%u\n", epoll_rateu32_5);
  printf("epollcomm tx_rateu32_6=%u\n", epoll_rateu32_6);

  info_hub.kv_del_u32("epollcomm", "tx_rateu32_1");
  info_hub.kv_del_u32("epollcomm", "tx_rateu32_2");
  info_hub.kv_del_u32("epollcomm", "tx_rateu32_3");
  info_hub.kv_del_u32("epollcomm", "tx_rateu32_4");
  info_hub.kv_del_u32("epollcomm", "tx_rateu32_5");
  info_hub.kv_del_u32("epollcomm", "tx_rateu32_6");
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


  //test km int test
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf1", /*tid*/1, /*rate*/10.1);
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf1", /*tid*/2, /*rate*/20.2);
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf1", /*tid*/3, /*rate*/30.3);
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf1", /*tid*/4, /*rate*/40.4);


  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf2", /*tid*/1, /*rate*/11.5);
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf2", /*tid*/2, /*rate*/22.6);
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf2", /*tid*/3, /*rate*/33.7);
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf2", /*tid*/4, /*rate*/44.8);

  double n_t1_lf1=info_hub.km_get_i2lf("netcombtransfer", "tx_tid_ratelf1", 1);
  double n_t1_lf2=info_hub.km_get_i2lf("netcombtransfer", "tx_tid_ratelf1", 2);
  double n_t1_lf3=info_hub.km_get_i2lf("netcombtransfer", "tx_tid_ratelf1", 2);
  double n_t1_lf4=info_hub.km_get_i2lf("netcombtransfer", "tx_tid_ratelf1", 3);

  double n_t2_lf1=info_hub.km_get_i2lf("netcombtransfer", "tx_tid_ratelf2", 1);
  double n_t2_lf2=info_hub.km_get_i2lf("netcombtransfer", "tx_tid_ratelf2", 2);
  double n_t2_lf3=info_hub.km_get_i2lf("netcombtransfer", "tx_tid_ratelf2", 3);
  double n_t2_lf4=info_hub.km_get_i2lf("netcombtransfer", "tx_tid_ratelf2", 4);

  printf(">>test3 km lf test\n");
  printf("netcombtransfer n_t1_lf1=%lf\n", n_t1_lf1);
  printf("netcombtransfer n_t1_lf2=%lf\n", n_t1_lf2);
  printf("netcombtransfer n_t1_lf3=%lf\n", n_t1_lf3);
  printf("netcombtransfer n_t1_lf4=%lf\n", n_t1_lf4);

  printf("netcombtransfer n_t2_lf1=%lf\n", n_t2_lf1);
  printf("netcombtransfer n_t2_lf2=%lf\n", n_t2_lf2);
  printf("netcombtransfer n_t2_lf3=%lf\n", n_t2_lf3);
  printf("netcombtransfer n_t2_lf4=%lf\n", n_t2_lf4);

  info_hub.km_del_i2lf("netcombtransfer", "tx_tid_ratelf1", 1);
  info_hub.km_del_i2lf("netcombtransfer", "tx_tid_ratelf1", 2);
  info_hub.km_del_i2lf("netcombtransfer", "tx_tid_ratelf1", 3);
  info_hub.km_del_i2lf("netcombtransfer", "tx_tid_ratelf1", 4);

  info_hub.km_del_i2lf("netcombtransfer", "tx_tid_ratelf2", 1);
  info_hub.km_del_i2lf("netcombtransfer", "tx_tid_ratelf2", 2);
  info_hub.km_del_i2lf("netcombtransfer", "tx_tid_ratelf2", 3);
  info_hub.km_del_i2lf("netcombtransfer", "tx_tid_ratelf2", 4);

  printf("\n");


  //test km uint32_t test
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/1, /*rate*/10);
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/2, /*rate*/20);
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/3, /*rate*/30);
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/4, /*rate*/40);


  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t2", /*tid*/1, /*rate*/11);
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t2", /*tid*/2, /*rate*/22);
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t2", /*tid*/3, /*rate*/33);
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t2", /*tid*/4, /*rate*/44);

  uint32_t n_t1_u32_t1=info_hub.km_get_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 1);
  uint32_t n_t1_u32_t2=info_hub.km_get_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 2);
  uint32_t n_t1_u32_t3=info_hub.km_get_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 2);
  uint32_t n_t1_u32_t4=info_hub.km_get_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 3);

  uint32_t n_t2_u32_t1=info_hub.km_get_i2u32("netcombtransfer", "tx_tid_rateu32_t2", 1);
  uint32_t n_t2_u32_t2=info_hub.km_get_i2u32("netcombtransfer", "tx_tid_rateu32_t2", 2);
  uint32_t n_t2_u32_t3=info_hub.km_get_i2u32("netcombtransfer", "tx_tid_rateu32_t2", 3);
  uint32_t n_t2_u32_t4=info_hub.km_get_i2u32("netcombtransfer", "tx_tid_rateu32_t2", 4);

  printf(">>test3 km u32 test\n");
  printf("netcombtransfer n_t1_u32_t1=%u\n", n_t1_u32_t1);
  printf("netcombtransfer n_t1_u32_t2=%u\n", n_t1_u32_t2);
  printf("netcombtransfer n_t1_u32_t3=%u\n", n_t1_u32_t3);
  printf("netcombtransfer n_t1_u32_t4=%u\n", n_t1_u32_t4);

  printf("netcombtransfer n_t2_u32_t1=%u\n", n_t2_u32_t1);
  printf("netcombtransfer n_t2_u32_t2=%u\n", n_t2_u32_t2);
  printf("netcombtransfer n_t2_u32_t3=%u\n", n_t2_u32_t3);
  printf("netcombtransfer n_t2_u32_t4=%u\n", n_t2_u32_t4);

  info_hub.km_del_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 1);
  info_hub.km_del_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 2);
  info_hub.km_del_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 3);
  info_hub.km_del_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 4);

  info_hub.km_del_i2u32("netcombtransfer", "tx_tid_rateu32_t2", 1);
  info_hub.km_del_i2u32("netcombtransfer", "tx_tid_rateu32_t2", 2);
  info_hub.km_del_i2u32("netcombtransfer", "tx_tid_rateu32_t2", 3);
  info_hub.km_del_i2u32("netcombtransfer", "tx_tid_rateu32_t2", 4);

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

  double epoll_ratelf2=info_hub.kv_get_lf("epollcomm", "tx_ratelf2");
  printf(">>kv lf test\n");
  printf("epollcomm tx_ratelf2=%lf\n", epoll_ratelf2);
  printf("++++++++++++++\n");

  uint32_t epoll_rateu32_2=info_hub.kv_get_u32("epollcomm", "tx_rateu32_2");
  printf(">>kv u32 test\n");
  printf("epollcomm tx_rateu32_2=%u\n", epoll_rateu32_2);
  printf("++++++++++++++\n");

  int n_t1_1=info_hub.km_get_i2i("netcombtransfer", "tx_tid_rate1", 1);
  printf(">>km int test\n");
  printf("netcombtransfer n_t1_1=%d\n", n_t1_1);
  printf("++++++++++++++\n");

  std::string n_r_1=info_hub.km_get_i2s("netcombtransfer", "send_task_dir", 1);
  printf(">>km str test\n");
  printf("netcombtransfer n_r_1=%s\n", n_r_1.c_str());
  printf("++++++++++++++\n");

  double n_t1_lf2=info_hub.km_get_i2lf("netcombtransfer", "tx_tid_ratelf1", 2);
  printf(">>km double test\n");
  printf("netcombtransfer n_t1_1=%lf\n", n_t1_lf2);
  printf("++++++++++++++\n");

  uint32_t n_t1_u32_t2=info_hub.km_get_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 2);
  printf(">>km u32 test\n");
  printf("netcombtransfer n_t1_1=%u\n", n_t1_u32_t2);
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

  //test kv lf test
  info_hub.kv_set_lf("epollcomm", "tx_ratelf1", 1.9);
  info_hub.kv_set_lf("epollcomm", "tx_ratelf1", 101.01);
  double epoll_ratelf=info_hub.kv_get_lf("epollcomm", "tx_ratelf1");
  printf(">>test1 kv lf test\n");
  printf("epollcomm tx_ratelf=%lf\n", epoll_ratelf);
  info_hub.kv_del_lf("epollcomm", "tx_ratelf1");

  //test kv u32 test
  info_hub.kv_set_u32("epollcomm", "tx_rateu32_1", 1);
  info_hub.kv_set_u32("epollcomm", "tx_rateu32_1", 109);
  uint32_t epoll_rateu32=info_hub.kv_get_u32("epollcomm", "tx_rateu32_1");
  printf(">>test1 kv u32 test\n");
  printf("epollcomm tx_rateu32=%u\n", epoll_rateu32);
  info_hub.kv_del_u32("epollcomm", "tx_rateu32_1");

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

  //test km lf test
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf1", /*tid*/1, /*rate*/1.9);
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf1", /*tid*/1, /*rate*/101.01);
  double n_t1_lf1=info_hub.km_get_i2lf("netcombtransfer", "tx_tid_ratelf1", 1);
  printf(">>test3 km lf test\n");
  printf("netcombtransfer n_t1_1=%lf\n", n_t1_lf1);
  info_hub.km_del_i2lf("netcombtransfer", "tx_tid_ratelf1", 1);

  //test km u32 test
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/1, /*rate*/1);
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/1, /*rate*/101);
  uint32_t n_t1_u32_t1=info_hub.km_get_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 1);
  printf(">>test3 km u32 test\n");
  printf("netcombtransfer n_t1_1=%u\n", n_t1_u32_t1);
  info_hub.km_del_i2u32("netcombtransfer", "tx_tid_rateu32_t1", 1);

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



{
  printf("========= WRONG TYPE TEST ========\n");
  //exist sample
  info_hub.kv_set_int("epollcomm", "tx_rate1", 1);
  info_hub.kv_set_str("epollcomm", "file1", "/a/b/c/d/file1.txt");
  info_hub.kv_set_lf("epollcomm", "tx_ratelf1", 1.9);
  info_hub.kv_set_u32("epollcomm", "tx_rateu32_1", 321);

  info_hub.km_set_i2i("netcombtransfer", "tx_tid_rate1", /*tid*/1, /*rate*/10);
  info_hub.km_set_i2s("netcombtransfer", "send_task_dir", /*taskid*/2, /*dir*/"/home/a/b/c/file2.txt");
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_ratelf1", /*tid*/1, /*rate*/10.9);
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/1, /*rate*/11);

  info_hub.ks_add_int("bigdatatransfer", "connected_tid", /*tid*/1);
  info_hub.ks_add_str("bigdatatransfer", "files", /*file*/"a/b/c/d/1.txt");

  //new set but wrong type
  info_hub.kv_set_int("epollcomm", "file1", 1);
  info_hub.kv_set_str("epollcomm", "tx_rate1", "/a/b/c/d/file1.txt");
  printf("---------------------\n");
  info_hub.kv_set_lf("epollcomm", "file1", 1.0);
  info_hub.kv_set_str("epollcomm", "tx_ratelf1", "/a/b/c/d/file1.txt");
  printf("---------------------\n");
  info_hub.kv_set_int("epollcomm", "tx_ratelf1", 1);
  info_hub.kv_set_lf("epollcomm", "tx_rate1", 1.0);
  printf("---------------------\n");
  info_hub.kv_set_u32("epollcomm", "tx_ratelf1", 1.9);
  info_hub.kv_set_lf("epollcomm", "tx_rateu32_1", 321);
  printf("+++++++++++++++++++++\n\n");

  //info_hub.kv_set_lf();

  info_hub.km_set_i2i("netcombtransfer", "send_task_dir", /*tid*/1, /*rate*/10);
  info_hub.km_set_i2s("netcombtransfer", "tx_tid_rate1", /*taskid*/2, /*dir*/"/home/a/b/c/file2.txt");
  printf("---------------------\n");
  info_hub.km_set_i2i("netcombtransfer", "tx_tid_ratelf1", /*tid*/1, /*rate*/10);
  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_rate1", /*tid*/1, /*rate*/10.1);
  printf("---------------------\n");
  info_hub.km_set_i2s("netcombtransfer", "tx_tid_ratelf1", /*taskid*/2, /*dir*/"/home/a/b/c/file2.txt");
  info_hub.km_set_i2lf("netcombtransfer", "send_task_dir", /*tid*/1, /*rate*/10.1);
  printf("---------------------\n");

  info_hub.km_set_i2lf("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/1, /*rate*/11);
  info_hub.km_set_i2u32("netcombtransfer", "tx_tid_ratelf1", /*tid*/1, /*rate*/10.9);


  printf("+++++++++++++++++++++\n\n");

  info_hub.ks_add_int("bigdatatransfer", "files", /*tid*/1);
  info_hub.ks_add_str("bigdatatransfer", "connected_tid", /*file*/"a/b/c/d/1.txt");
  printf("+++++++++++++++++++++\n\n");
}

  return 0;
}
