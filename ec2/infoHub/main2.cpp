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

std::shared_ptr<ec2::infoHub> infohub_instance =\
                        ec2::infoHub::getInstance();

int main(void)
{

  printf("========= KV TEST ========\n");
  //test kv int test
  infohub_instance->value_set("epollcomm", "tx_rate1", 100);
  infohub_instance->value_set("epollcomm", "tx_rate2", 200);
  infohub_instance->value_set("epollcomm", "tx_rate3", 300);
  infohub_instance->value_set("epollcomm", "tx_rate4", 400);
  infohub_instance->value_set("epollcomm", "tx_rate5", 500);
  infohub_instance->value_set("epollcomm", "tx_rate6", 600);

  int epoll_rate=infohub_instance->value_get<int>("epollcomm", "tx_rate2");
  printf(">>test1 kv int test\n");
  printf("epollcomm tx_rate=%d\n", epoll_rate);

  infohub_instance->value_del("epollcomm", "tx_rate1");
  infohub_instance->value_del("epollcomm", "tx_rate2");
  printf("\n");

  //test kv string test
  infohub_instance->value_set("epollcomm", "file1", "/a/b/c/d/file1.txt");
  infohub_instance->value_set("epollcomm", "file2", "/a/b/c/d/file2.txt");
  infohub_instance->value_set("epollcomm", "file3", "/a/b/c/d/file3.txt");
  infohub_instance->value_set("epollcomm", "file4", "/a/b/c/d/file4.txt");
  infohub_instance->value_set("epollcomm", "file5", "/a/b/c/d/file5.txt");
  infohub_instance->value_set("epollcomm", "file6", "/a/b/c/d/file6.txt");

  string epoll_file=infohub_instance->value_get<std::string>("epollcomm", "file2");
  printf(">>test2 kv str test\n");
  printf("epollcomm file=%s\n", epoll_file.c_str());

  infohub_instance->value_del("epollcomm", "file2");
  printf("\n");

  //test kv double test
  infohub_instance->value_set("epollcomm", "lf_rate1", 1.1);
  infohub_instance->value_set("epollcomm", "lf_rate2", 1.2);
  infohub_instance->value_set("epollcomm", "lf_rate3", 1.3);
  infohub_instance->value_set("epollcomm", "lf_rate4", 1.4);
  infohub_instance->value_set("epollcomm", "lf_rate5", 1.5);
  infohub_instance->value_set("epollcomm", "lf_rate6", 1.6);

  double epoll_lf_rate=infohub_instance->value_get<double>("epollcomm", "lf_rate2");
  printf(">>test2 kv lf test\n");
  printf("epollcomm lf_rate2=%lf\n", epoll_lf_rate);

  infohub_instance->value_del("epollcomm", "lf_rate2");
  printf("\n");

  //test kv uint32_t test
  infohub_instance->value_set("epollcomm", "u32_rate1", (uint32_t)1);
  infohub_instance->value_set("epollcomm", "u32_rate2", (uint32_t)9);
  infohub_instance->value_set("epollcomm", "u32_rate3", (uint32_t)2);
  infohub_instance->value_set("epollcomm", "u32_rate4", (uint32_t)8);
  infohub_instance->value_set("epollcomm", "u32_rate5", (uint32_t)3);
  infohub_instance->value_set("epollcomm", "u32_rate6", (uint32_t)7);

  uint32_t epoll_u32_rate1=infohub_instance->value_get<uint32_t>("epollcomm", "u32_rate1");
  uint32_t epoll_u32_rate2=infohub_instance->value_get<uint32_t>("epollcomm", "u32_rate2");
  uint32_t epoll_u32_rate3=infohub_instance->value_get<uint32_t>("epollcomm", "u32_rate3");
  uint32_t epoll_u32_rate4=infohub_instance->value_get<uint32_t>("epollcomm", "u32_rate4");
  uint32_t epoll_u32_rate5=infohub_instance->value_get<uint32_t>("epollcomm", "u32_rate5");
  uint32_t epoll_u32_rate6=infohub_instance->value_get<uint32_t>("epollcomm", "u32_rate6");
  printf(">>test2 kv uint32_t test\n");
  printf("epollcomm u32_rate1=%u\n", epoll_u32_rate1);
  printf("epollcomm u32_rate2=%u\n", epoll_u32_rate2);
  printf("epollcomm u32_rate3=%u\n", epoll_u32_rate3);
  printf("epollcomm u32_rate4=%u\n", epoll_u32_rate4);
  printf("epollcomm u32_rate5=%u\n", epoll_u32_rate5);
  printf("epollcomm u32_rate6=%u\n", epoll_u32_rate6);

  infohub_instance->value_del("epollcomm", "u32_rate1");
  infohub_instance->value_del("epollcomm", "u32_rate2");
  infohub_instance->value_del("epollcomm", "u32_rate3");
  infohub_instance->value_del("epollcomm", "u32_rate4");
  infohub_instance->value_del("epollcomm", "u32_rate5");
  infohub_instance->value_del("epollcomm", "u32_rate6");
  printf("\n");

  printf("========= KM TEST ========\n");
  //test km int test
  infohub_instance->table_set("netcombtransfer", "tx_tid_rate1", /*tid*/1, /*rate*/10999);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rate1", /*tid*/2, /*rate*/20999);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rate1", /*tid*/3, /*rate*/30999);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rate1", /*tid*/4, /*rate*/40999);
                  
                  
  infohub_instance->table_set("netcombtransfer", "tx_tid_rate2", /*tid*/1, /*rate*/11999);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rate2", /*tid*/2, /*rate*/22999);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rate2", /*tid*/3, /*rate*/33999);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rate2", /*tid*/4, /*rate*/44999);

  int n_t1_1=infohub_instance->table_get<int>("netcombtransfer", "tx_tid_rate1", 1);
  int n_t1_2=infohub_instance->table_get<int>("netcombtransfer", "tx_tid_rate1", 2);
  int n_t1_3=infohub_instance->table_get<int>("netcombtransfer", "tx_tid_rate1", 2);
  int n_t1_4=infohub_instance->table_get<int>("netcombtransfer", "tx_tid_rate1", 3);
                             
  int n_t2_1=infohub_instance->table_get<int>("netcombtransfer", "tx_tid_rate2", 1);
  int n_t2_2=infohub_instance->table_get<int>("netcombtransfer", "tx_tid_rate2", 2);
  int n_t2_3=infohub_instance->table_get<int>("netcombtransfer", "tx_tid_rate2", 3);
  int n_t2_4=infohub_instance->table_get<int>("netcombtransfer", "tx_tid_rate2", 4);

  printf(">>test3 km int test\n");
  printf("netcombtransfer n_t1_1=%d\n", n_t1_1);
  printf("netcombtransfer n_t1_2=%d\n", n_t1_2);
  printf("netcombtransfer n_t1_3=%d\n", n_t1_3);
  printf("netcombtransfer n_t1_4=%d\n", n_t1_4);

  printf("netcombtransfer n_t2_1=%d\n", n_t2_1);
  printf("netcombtransfer n_t2_2=%d\n", n_t2_2);
  printf("netcombtransfer n_t2_3=%d\n", n_t2_3);
  printf("netcombtransfer n_t2_4=%d\n", n_t2_4);

  infohub_instance->table_del("netcombtransfer", "tx_tid_rate1", 1);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate1", 2);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate1", 3);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate1", 4);

  infohub_instance->table_del("netcombtransfer", "tx_tid_rate2", 1);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate2", 2);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate2", 3);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate2", 4);

  printf("\n");


  //test km lf test
  infohub_instance->table_set("netcombtransfer", "tx_tid_ratelf1", /*tid*/1, /*rate*/10999.001);
  infohub_instance->table_set("netcombtransfer", "tx_tid_ratelf1", /*tid*/2, /*rate*/20999.002);
  infohub_instance->table_set("netcombtransfer", "tx_tid_ratelf1", /*tid*/3, /*rate*/30999.003);
  infohub_instance->table_set("netcombtransfer", "tx_tid_ratelf1", /*tid*/4, /*rate*/40999.004);
                  
                  
  infohub_instance->table_set("netcombtransfer", "tx_tid_ratelf2", /*tid*/1, /*rate*/11999.001);
  infohub_instance->table_set("netcombtransfer", "tx_tid_ratelf2", /*tid*/2, /*rate*/22999.002);
  infohub_instance->table_set("netcombtransfer", "tx_tid_ratelf2", /*tid*/3, /*rate*/33999.003);
  infohub_instance->table_set("netcombtransfer", "tx_tid_ratelf2", /*tid*/4, /*rate*/44999.004);

  double n_t1_lf1=infohub_instance->table_get<double>("netcombtransfer", "tx_tid_ratelf1", 1);
  double n_t1_lf2=infohub_instance->table_get<double>("netcombtransfer", "tx_tid_ratelf1", 2);
  double n_t1_lf3=infohub_instance->table_get<double>("netcombtransfer", "tx_tid_ratelf1", 2);
  double n_t1_lf4=infohub_instance->table_get<double>("netcombtransfer", "tx_tid_ratelf1", 3);
  
  double n_t2_lf1=infohub_instance->table_get<double>("netcombtransfer", "tx_tid_ratelf2", 1);
  double n_t2_lf2=infohub_instance->table_get<double>("netcombtransfer", "tx_tid_ratelf2", 2);
  double n_t2_lf3=infohub_instance->table_get<double>("netcombtransfer", "tx_tid_ratelf2", 3);
  double n_t2_lf4=infohub_instance->table_get<double>("netcombtransfer", "tx_tid_ratelf2", 4);

  printf(">>test3 km int test\n");
  printf("netcombtransfer n_t1_1=%lf\n", n_t1_lf1);
  printf("netcombtransfer n_t1_2=%lf\n", n_t1_lf2);
  printf("netcombtransfer n_t1_3=%lf\n", n_t1_lf3);
  printf("netcombtransfer n_t1_4=%lf\n", n_t1_lf4);

  printf("netcombtransfer n_t2_1=%lf\n", n_t2_lf1);
  printf("netcombtransfer n_t2_2=%lf\n", n_t2_lf2);
  printf("netcombtransfer n_t2_3=%lf\n", n_t2_lf3);
  printf("netcombtransfer n_t2_4=%lf\n", n_t2_lf4);

  infohub_instance->table_del("netcombtransfer", "tx_tid_rate1lf", 1);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate1lf", 2);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate1lf", 3);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate1lf", 4);

  infohub_instance->table_del("netcombtransfer", "tx_tid_rate2lf", 1);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate2lf", 2);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate2lf", 3);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rate2lf", 4);

  printf("\n");

  //test km u32 test
  infohub_instance->table_set("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/1, /*rate*/(uint32_t)10900);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/2, /*rate*/(uint32_t)20900);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/3, /*rate*/(uint32_t)30900);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rateu32_t1", /*tid*/4, /*rate*/(uint32_t)40900);
                  
                  
  infohub_instance->table_set("netcombtransfer", "tx_tid_rateu32_t2", /*tid*/1, /*rate*/(uint32_t)11900);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rateu32_t2", /*tid*/2, /*rate*/(uint32_t)22900);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rateu32_t2", /*tid*/3, /*rate*/(uint32_t)33900);
  infohub_instance->table_set("netcombtransfer", "tx_tid_rateu32_t2", /*tid*/4, /*rate*/(uint32_t)44900);

  uint32_t n_t1_u32_t1=infohub_instance->table_get<uint32_t>("netcombtransfer", "tx_tid_rateu32_t1", 1);
  uint32_t n_t1_u32_t2=infohub_instance->table_get<uint32_t>("netcombtransfer", "tx_tid_rateu32_t1", 2);
  uint32_t n_t1_u32_t3=infohub_instance->table_get<uint32_t>("netcombtransfer", "tx_tid_rateu32_t1", 2);
  uint32_t n_t1_u32_t4=infohub_instance->table_get<uint32_t>("netcombtransfer", "tx_tid_rateu32_t1", 3);
  
  uint32_t n_t2_u32_t1=infohub_instance->table_get<uint32_t>("netcombtransfer", "tx_tid_rateu32_t2", 1);
  uint32_t n_t2_u32_t2=infohub_instance->table_get<uint32_t>("netcombtransfer", "tx_tid_rateu32_t2", 2);
  uint32_t n_t2_u32_t3=infohub_instance->table_get<uint32_t>("netcombtransfer", "tx_tid_rateu32_t2", 3);
  uint32_t n_t2_u32_t4=infohub_instance->table_get<uint32_t>("netcombtransfer", "tx_tid_rateu32_t2", 4);

  printf(">>test3 km u32 test\n");
  printf("netcombtransfer n_t1_1=%u\n", n_t1_u32_t1);
  printf("netcombtransfer n_t1_2=%u\n", n_t1_u32_t2);
  printf("netcombtransfer n_t1_3=%u\n", n_t1_u32_t3);
  printf("netcombtransfer n_t1_4=%u\n", n_t1_u32_t4);

  printf("netcombtransfer n_t2_1=%u\n", n_t2_u32_t1);
  printf("netcombtransfer n_t2_2=%u\n", n_t2_u32_t2);
  printf("netcombtransfer n_t2_3=%u\n", n_t2_u32_t3);
  printf("netcombtransfer n_t2_4=%u\n", n_t2_u32_t4);

  infohub_instance->table_del("netcombtransfer", "tx_tid_rateu32_t1", 1);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rateu32_t1", 2);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rateu32_t1", 3);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rateu32_t1", 4);

  infohub_instance->table_del("netcombtransfer", "tx_tid_rateu32_t2", 1);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rateu32_t2", 2);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rateu32_t2", 3);
  infohub_instance->table_del("netcombtransfer", "tx_tid_rateu32_t2", 4);

  printf("\n");

  //test km str test
  infohub_instance->table_set("netcombtransfer", "send_task_dir", /*taskid*/1, /*dir*/"/home/a/b/c/file1.txt");
  infohub_instance->table_set("netcombtransfer", "send_task_dir", /*taskid*/2, /*dir*/"/home/a/b/c/file2.txt");
  infohub_instance->table_set("netcombtransfer", "send_task_dir", /*taskid*/3, /*dir*/"/home/a/b/c/file3.txt");
  infohub_instance->table_set("netcombtransfer", "send_task_dir", /*taskid*/4, /*dir*/"/home/a/b/c/file4.txt");


  infohub_instance->table_set("netcombtransfer", "recv_task_dir", /*taskid*/1, /*dir*/"/home/a/b/c/file11.txt");
  infohub_instance->table_set("netcombtransfer", "recv_task_dir", /*taskid*/2, /*dir*/"/home/a/b/c/file22.txt");
  infohub_instance->table_set("netcombtransfer", "recv_task_dir", /*taskid*/3, /*dir*/"/home/a/b/c/file33.txt");
  infohub_instance->table_set("netcombtransfer", "recv_task_dir", /*taskid*/4, /*dir*/"/home/a/b/c/file44.txt");

  std::string n_r_1=infohub_instance->table_get<std::string>("netcombtransfer", "send_task_dir", 1);
  std::string n_r_2=infohub_instance->table_get<std::string>("netcombtransfer", "send_task_dir", 2);
  std::string n_r_3=infohub_instance->table_get<std::string>("netcombtransfer", "send_task_dir", 3);
  std::string n_r_4=infohub_instance->table_get<std::string>("netcombtransfer", "send_task_dir", 4);

  std::string n_s_1=infohub_instance->table_get<std::string>("netcombtransfer", "recv_task_dir", 1);
  std::string n_s_2=infohub_instance->table_get<std::string>("netcombtransfer", "recv_task_dir", 2);
  std::string n_s_3=infohub_instance->table_get<std::string>("netcombtransfer", "recv_task_dir", 3);
  std::string n_s_4=infohub_instance->table_get<std::string>("netcombtransfer", "recv_task_dir", 4);

  printf(">>test4 km str test\n");
  printf("netcombtransfer n_r_1=%s\n", n_r_1.c_str());
  printf("netcombtransfer n_r_2=%s\n", n_r_2.c_str());
  printf("netcombtransfer n_r_3=%s\n", n_r_3.c_str());
  printf("netcombtransfer n_r_4=%s\n", n_r_4.c_str());
                                  
  printf("netcombtransfer n_s_1=%s\n", n_s_1.c_str());
  printf("netcombtransfer n_s_2=%s\n", n_s_2.c_str());
  printf("netcombtransfer n_s_3=%s\n", n_s_3.c_str());
  printf("netcombtransfer n_s_4=%s\n", n_s_4.c_str());

  infohub_instance->table_del("netcombtransfer", "send_task_dir", 1);
  infohub_instance->table_del("netcombtransfer", "send_task_dir", 2);
  infohub_instance->table_del("netcombtransfer", "send_task_dir", 3);
  infohub_instance->table_del("netcombtransfer", "send_task_dir", 4);
           
  infohub_instance->table_del("netcombtransfer", "recv_task_dir", 1);
  infohub_instance->table_del("netcombtransfer", "recv_task_dir", 2);
  infohub_instance->table_del("netcombtransfer", "recv_task_dir", 3);
  infohub_instance->table_del("netcombtransfer", "recv_task_dir", 4);

  printf("\n");

  printf("========= KS TEST ========\n");
  //test ks int test
  infohub_instance->set_add("bigdatatransfer", "connected_tid", /*tid*/1);
  infohub_instance->set_add("bigdatatransfer", "connected_tid", /*tid*/2);
  infohub_instance->set_add("bigdatatransfer", "connected_tid", /*tid*/3);

  bool b_c_1=infohub_instance->set_ism("bigdatatransfer", "connected_tid", 1);
  bool b_c_2=infohub_instance->set_ism("bigdatatransfer", "connected_tid", 2);
  bool b_c_3=infohub_instance->set_ism("bigdatatransfer", "connected_tid", 3);
  bool b_c_4=infohub_instance->set_ism("bigdatatransfer", "connected_tid", 4);

  printf(">>test5 km int test\n");
  printf("bigdatatransfer has b_c_1=%d(true/false)\n", b_c_1);
  printf("bigdatatransfer has b_c_2=%d(true/false)\n", b_c_2);
  printf("bigdatatransfer has b_c_3=%d(true/false)\n", b_c_3);
  printf("bigdatatransfer has b_c_4=%d(true/false)\n", b_c_4);

  infohub_instance->set_del("bigdatatransfer", "connected_tid", 1);
  infohub_instance->set_del("bigdatatransfer", "connected_tid", 2);
  infohub_instance->set_del("bigdatatransfer", "connected_tid", 3);
  //infohub_instance.ks_del_int("bigdatatransfer", "connected_tid", 4);

  printf("\n");

  //test ks str test
  infohub_instance->set_add("bigdatatransfer", "files", /*file*/"a/b/c/d/1.txt");
  infohub_instance->set_add("bigdatatransfer", "files", /*file*/"a/b/c/d/2.txt");
  infohub_instance->set_add("bigdatatransfer", "files", /*file*/"a/b/c/d/3.txt");

  bool b_f_1=infohub_instance->set_ism("bigdatatransfer", "files", "a/b/c/d/1.txt");
  bool b_f_2=infohub_instance->set_ism("bigdatatransfer", "files", "a/b/c/d/2.txt");
  bool b_f_3=infohub_instance->set_ism("bigdatatransfer", "files", "a/b/c/d/3.txt");
  bool b_f_4=infohub_instance->set_ism("bigdatatransfer", "files", "a/b/c/d/4.txt");

  printf(">>test6 km str test\n");
  printf("bigdatatransfer has b_c_1=%d(true/false)\n", b_f_1);
  printf("bigdatatransfer has b_c_2=%d(true/false)\n", b_f_2);
  printf("bigdatatransfer has b_c_3=%d(true/false)\n", b_f_3);
  printf("bigdatatransfer has b_c_4=%d(true/false)\n", b_f_4);

  infohub_instance->set_del("bigdatatransfer", "files", /*file*/"a/b/c/d/1.txt");
  infohub_instance->set_del("bigdatatransfer", "files", /*file*/"a/b/c/d/2.txt");
  infohub_instance->set_del("bigdatatransfer", "files", /*file*/"a/b/c/d/3.txt");
  //infohub_instance.ks_del_str("bigdatatransfer", "files", /*file*/"a/b/c/d/4.txt");

  printf("\n");

  return 0;
}
