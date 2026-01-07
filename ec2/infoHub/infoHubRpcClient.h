
#ifndef __INFO_HUB_RPC__
#define __INFO_HUB_RPC__

#include "rpc/client.h"

class infoHubRpcClient
{
private:
    std::shared_ptr<rpc::client> _infohub_rpcclient;
    //std::shared_ptr<rpc::client> _infohub_rpcclient("127.0.0.1", rpc::constants::DEFAULT_PORT);

public:
    infoHubRpcClient()
    {
    }

    void init(int port=8088)
    {
        if(nullptr == _infohub_rpcclient)
        {
            _infohub_rpcclient = std::shared_ptr<rpc::client>(new rpc::client("127.0.0.1", port));
        }
    }

    void init(std::string ipstr, int port)
    {
        if(nullptr == _infohub_rpcclient)
        {
            _infohub_rpcclient = std::shared_ptr<rpc::client>(new rpc::client(ipstr, port));
        }
    }

    // initial:1, connected:2, disconnected:3, reset:4
    int get_connection_state()
    {
        rpc::client::connection_state cs = _infohub_rpcclient->get_connection_state();
        int cs_ret = 0;
        switch(cs){
            case rpc::client::connection_state::initial :
                { cs_ret = 1; }
                break;
            case rpc::client::connection_state::connected :
                { cs_ret = 2; }
                break;
            case rpc::client::connection_state::disconnected :
                { cs_ret = 3; }
                break;
            case rpc::client::connection_state::reset :
                { cs_ret = 4; }
                break;
            default:
                //unknown connection state
                ;
        }
        return cs_ret;
    }

    //-------------  kv rpc func ----------------

    bool    value_set(std::string module, std::string key, int value)
    {
        return _infohub_rpcclient->call("value_set_int", module, key, value).as<bool>();
    }

    bool    value_set(std::string module, std::string key, std::string value)
    {
        return _infohub_rpcclient->call("value_set_str", module, key, value).as<bool>();
    }

    bool    value_set(std::string module, std::string key, double value)
    {
        return _infohub_rpcclient->call("value_set_lf", module, key, value).as<bool>();
    }

    bool    value_set(std::string module, std::string key, uint32_t value)
    {
        return _infohub_rpcclient->call("value_set_u32", module, key, value).as<bool>();
    }


    bool    value_del(std::string module, std::string key)
    {
        return _infohub_rpcclient->call("value_del", module, key).as<bool>();
    }


    template<typename T>
    T value_get(std::string module, std::string key) 
    {
    //template<typename T>
    //T value_get(std::string module, std::string key) { }
        T value;
        value_get(module, key, value);
        return value;
    }

    bool    value_get(std::string module, std::string key, int &value)
    {
        value =  _infohub_rpcclient->call("value_get_int", module, key).as<int>();
        if(value == -1 ) return false ; 
        return true;
    }

    bool    value_get(std::string module, std::string key, std::string &value)
    {
        value =  _infohub_rpcclient->call("value_get_str", module, key).as<std::string>();
        if(value == "" ) return false ;
        return true;
    }

    bool    value_get(std::string module, std::string key, double &value)
    {
        value =  _infohub_rpcclient->call("value_get_lf", module, key).as<double>();
        if(value == -1.0 ) return false ; 
        return true;
    }

    bool    value_get(std::string module, std::string key, uint32_t &value)
    {
        value =  _infohub_rpcclient->call("value_get_u32", module, key).as<uint32_t>();
        if(value == -1 ) return false ; 
        return true;
    }
    
    //-------------  km rpc func ----------------
    bool    table_set(std::string module, std::string key, int field, int value)
    {
        return _infohub_rpcclient->call("table_set_int", module, key, field, value).as<bool>();
    }

    bool    table_set(std::string module, std::string key, int field, std::string value)
    {
        return _infohub_rpcclient->call("table_set_str", module, key, field, value).as<bool>();
    }

    bool    table_set(std::string module, std::string key, int field, double value)
    {
        return _infohub_rpcclient->call("table_set_lf", module, key, field, value).as<bool>();
    }

    bool    table_set(std::string module, std::string key, int field, uint32_t value)
    {
        return _infohub_rpcclient->call("table_set_u32", module, key, field, value).as<bool>();
    }


    bool    table_del(std::string module, std::string key, int field)
    {
        return _infohub_rpcclient->call("table_del", module, key, field).as<bool>();
    }


    template<typename T>
    T table_get(std::string module, std::string key, int field) 
    {
        T value;
        table_get(module, key, field, value);
        return value;
    }

    bool    table_get(std::string module, std::string key, int field, int &value)
    {
        value =  _infohub_rpcclient->call("table_get_int", module, key, field).as<int>();
        if(value == -1 ) return false ; 
        return true;
    }

    bool    table_get(std::string module, std::string key, int field, std::string &value)
    {
        value =  _infohub_rpcclient->call("table_get_str", module, key, field).as<std::string>();
        if(value == "" ) return false ;
        return true;
    }

    bool    table_get(std::string module, std::string key, int field, double &value)
    {
        value =  _infohub_rpcclient->call("table_get_lf", module, key, field).as<double>();
        if(value == -1.0 ) return false ; 
        return true;
    }

    bool    table_get(std::string module, std::string key, int field, uint32_t &value)
    {
        value =  _infohub_rpcclient->call("table_get_u32", module, key, field).as<uint32_t>();
        if(value == -1 ) return false ; 
        return true;
    }

    int    table_len(std::string module, std::string key)
    {
        return  _infohub_rpcclient->call("table_len", module, key).as<int>();
    }
    


};


#endif
