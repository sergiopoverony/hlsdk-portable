# PlayStation 4

Game libraries for the experimental Xash3D FWGS PS4 port, built with
[OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain).

```
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis/PS4Toolchain
scripts/ps4/build.sh              # build/ps4/valve/{cl_dlls,dlls}/*.prx
scripts/ps4/build.sh 192.168.0.10 # same, and upload to /data/xash/valve on the console (GoldHEN FTP)
```

Other mods are configured as usual, e.g.
`WAF_CONFIGURE="--gamedir=gearbox --server-library-name=opfor" scripts/ps4/build.sh` on the matching branch.

`ps4_cwd.c` and `scripts/waifulib/ps4.py` are shared with the engine repository.
