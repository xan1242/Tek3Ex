# Print current LBA to TTY console

Use a TTY-enabled emulator such as DuckStation or PCSX-R.

For DuckStation - enable "Enable TTY Logging" in the BIOS settings

For PCSX-R - enable interpreter mode, debugger and console output in CPU settings

After that, enable the cheat below and you should see numbers appear in the console. It will only display a number on a new line for each file it loads (this is due to the format string only being `%d\n`).

## Tekken 3 SCPS-45213 / SLPS-01300 (NTSC-J)

Gameshark form:

```
800281E4 0000
800281E6 AF9F
800281E8 0000
800281EA 8605
800281EC 8002
800281EE 3C04
800281F0 EA1F
800281F2 0C01
800281F4 81D4
800281F6 3484
800281F8 0000
800281FA 8F9F
800281FC 800A
800281FE 3C04
80028200 440C
80028202 0802
80028204 0988
80028206 2484
8006C2A8 A079
8006C2AA 0C00
```

Wide (32-bit) form of cheat:

```
800281E4 AF9F0000  // sw ra, 0(gp)
800281E8 86050000  // lh a1, 0(s0)
800281EC 3C048002  // lui a0, 0x8002
800281F0 0C01EA1F  // jal 0x8007A87C       // printf
800281F4 348481D4  // ori a0, a0, 0x81D4
800281F8 8F9F0000  // lw ra, 0(gp)
800281FC 3C04800A  // lui a0, 0x800A
80028200 0802440C  // j 0x80091030         // continue to intercepted func
80028204 24840988  // addiu a0, a0, 0x0988
8006C2A8 0C00A079  // jal 0x800281E4
```

## TODO: Other regions!
