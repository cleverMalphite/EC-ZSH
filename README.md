#EC2-develop


# EC-ZSH

## Dynamic ini for new terminals

The config loader now auto-creates an ini file when the given path does not exist.

Default behavior:
- Device ID is parsed from the ini filename digits (for example `SysConfig103.ini` -> `DeviceID=103`).
- If filename has no digits, default `DeviceID=101` is used.
- A full default config is generated, including Main, MRUDP, RBUDP, BigDataTransfer, DTU, Display, FirstOutDoorTest and AutoConn sections.

Runtime override via environment variables (optional):
- `EC2_DEVICE_ID`
- `EC2_REMOTE_TID`
- `EC2_AUTOCONN_ROLE` (1 or 2)
- `EC2_LOCAL_IP`
- `EC2_REMOTE_IP`
- `EC2_LOCAL_PORT`
- `EC2_REMOTE_PORT`

Example:

```bash
export EC2_DEVICE_ID=103
export EC2_REMOTE_TID=201
export EC2_LOCAL_IP=10.0.0.10
export EC2_REMOTE_IP=10.0.0.11
export EC2_LOCAL_PORT=4000
export EC2_REMOTE_PORT=4001
./ec_singletransfer ../log/new_terminal.log "<inifilepath>../SysConfig103.ini</inifilepath>..."
```

If `../SysConfig103.ini` does not exist, it is created automatically before loading.
