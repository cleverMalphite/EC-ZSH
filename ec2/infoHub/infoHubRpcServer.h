
#ifndef __INFO_HUB_RPC__
#define __INFO_HUB_RPC__

#include "rpc/server.h"
#include "rpc/this_handler.h"
#include "infoHub.h"

class infoHubRpcServer
{
private:
    std::shared_ptr<ec2::infoHub> _infohub_instance;
    //rpc::server _infohub_rpcserver(8888);
    std::shared_ptr<rpc::server> _infohub_rpcserver;

public:
    infoHubRpcServer()
    {
    }

    void init(std::shared_ptr<ec2::infoHub> infohub_instance, int port=8088){
        _infohub_instance = infohub_instance;
        if(nullptr == _infohub_rpcserver)
        {
            _infohub_rpcserver = std::shared_ptr<rpc::server>(new rpc::server(port));
            bind();
        }
    }
    void bind(){
        bind_kv_func();
        bind_km_func();
    }

    void bind_kv_func(){
        _infohub_rpcserver->bind("value_set_int",
                                [this](std::string module, std::string key, int value) -> bool {
                                    return _infohub_instance->value_set(module, key, value);
                                } ); 
        _infohub_rpcserver->bind("value_set_str",
                                [this](std::string module, std::string key, std::string value) -> bool {
                                    return _infohub_instance->value_set(module, key, value);
                                } ); 
        _infohub_rpcserver->bind("value_set_lf",
                                [this](std::string module, std::string key, double value) -> bool {
                                    return _infohub_instance->value_set(module, key, value);
                                } ); 
        _infohub_rpcserver->bind("value_set_u32",
                                [this](std::string module, std::string key, uint32_t value) -> bool {
                                    return _infohub_instance->value_set(module, key, value);
                                } ); 

        _infohub_rpcserver->bind("value_del", 
                                [this](std::string module, std::string key) -> bool {
                                    return _infohub_instance->value_del(module, key);
                                }); 

        _infohub_rpcserver->bind("value_get_int", 
                                [this](std::string module, std::string key) -> int {
                                    int value = _infohub_instance->value_get<int>(module, key);
                                    return value;
                                }); 
        _infohub_rpcserver->bind("value_get_str", 
                                [this](std::string module, std::string key) -> std::string {
                                    std::string value = _infohub_instance->value_get<std::string>(module, key);
                                    return value;
                                }); 
        _infohub_rpcserver->bind("value_get_lf", 
                                [this](std::string module, std::string key) -> double {
                                    double value = _infohub_instance->value_get<double>(module, key);
                                    return value;
                                }); 
        _infohub_rpcserver->bind("value_get_u32", 
                                [this](std::string module, std::string key) -> uint32_t {
                                    uint32_t value = _infohub_instance->value_get<uint32_t>(module, key);
                                    return value;
                                }); 
    }
    
    void bind_km_func(){
        _infohub_rpcserver->bind("table_set_int",
                                [this](std::string module, std::string key, int field, int value) -> bool {
                                    return _infohub_instance->table_set(module, key, field, value);
                                } ); 
        _infohub_rpcserver->bind("table_set_str",
                                [this](std::string module, std::string key, int field, std::string value) -> bool {
                                    return _infohub_instance->table_set(module, key, field, value);
                                } ); 
        _infohub_rpcserver->bind("table_set_lf",
                                [this](std::string module, std::string key, int field, double value) -> bool {
                                    return _infohub_instance->table_set(module, key, field, value);
                                } ); 
        _infohub_rpcserver->bind("table_set_u32",
                                [this](std::string module, std::string key, int field, uint32_t value) -> bool {
                                    return _infohub_instance->table_set(module, key, field, value);
                                } ); 

        _infohub_rpcserver->bind("table_del", 
                                [this](std::string module, std::string key, int field) -> bool {
                                    return _infohub_instance->table_del(module, key, field);
                                }); 

        _infohub_rpcserver->bind("table_get_int", 
                                [this](std::string module, std::string key, int field) -> int {
                                    int value = _infohub_instance->table_get<int>(module, key, field);
                                    return value;
                                }); 
        _infohub_rpcserver->bind("table_get_str", 
                                [this](std::string module, std::string key, int field) -> std::string {
                                    std::string value = _infohub_instance->table_get<std::string>(module, key, field);
                                    return value;
                                }); 
        _infohub_rpcserver->bind("table_get_lf", 
                                [this](std::string module, std::string key, int field) -> double {
                                    double value = _infohub_instance->table_get<double>(module, key, field);
                                    return value;
                                }); 
        _infohub_rpcserver->bind("table_get_u32", 
                                [this](std::string module, std::string key, int field) -> uint32_t {
                                    uint32_t value = _infohub_instance->table_get<uint32_t>(module, key, field);
                                    return value;
                                }); 
        _infohub_rpcserver->bind("table_len", 
                                [this](std::string module, std::string key) -> int {
                                    int len = _infohub_instance->table_len(module, key);
                                    return len;
                                }); 
    }

    void run(){
        _infohub_rpcserver->run();
    }

    void async_run(std::size_t worker_threads = 1){
        _infohub_rpcserver->async_run(worker_threads);
    }

};


#endif
