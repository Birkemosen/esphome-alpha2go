# BLE traffic reference: ALPHA2 GO

Frames observed on the BLE link of an ALPHA2 GO 25-90 while settings were changed from a phone (Grundfos GO), captured with the Android Bluetooth HCI snoop log on 2026-10-28. These observations are the basis for the class 10 SET frames, object layouts and the proportional pressure curve in this component.

Only the frames to and from the pump are kept. Left out: the pump's serial number, production code and ECU id (class 7 ids 9, 10, 100), firmware update objects (202/x), event and trend logs (53/x, 88/x) and operation history (93/1).

Frame format: `[SD][LEN][DA][SA][APDU...][CRC_hi][CRC_lo]`, requests `27 .. E7 F8`, replies `24 .. F8 E7`. Frames are written in pieces of at most 20 bytes (ATT MTU 23); the pump does not answer a longer frame sent in one write. Class 10 SET APDU: `0A [0x80 | len] id sub_hi sub_lo type_hi type_lo version size(3) data`. Set points in Pa use 9804 Pa/m.

## Session 1

### Writes (class 10 SET)

Every write was answered with ACK OK (`24 .. 0A 01 00`).

| Time | Change made | Object | Value | Frame sent |
| --- | --- | --- | --- | --- |
| 12:42:27.038 | Phone connects | 202/9 type 292 v1 | ECU select = 0 | `27 11 E7 F8 0A 8D CA 00 09 01 24 01 00 00 04 00 00 00 00 50 1D` |
| 12:42:27.232 | Phone connects | 202/9 type 292 v1 | ECU select = 1 | `27 11 E7 F8 0A 8D CA 00 09 01 24 01 00 00 04 00 00 00 01 40 3C` |
| 12:42:59.716 | CP -> CP AUTOADAPT | 86/10 type 303 v1 | source 0, operation mode 6, control mode 14 (CP AUTOADAPT), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 0E 7F FF FF FF 87 C7` |
| 12:42:59.759 | CP -> CP AUTOADAPT | 86/21 type 301 v1 | default 49020, min 24510, max 73530, resulting min 24510, controller 0.1 / 0.95 / 1 (written back unchanged) | `27 29 E7 F8 0A A5 56 00 15 01 2D 01 00 00 1C 47 3F 7C 00 46 BF 7C 00 47 8F 9D 00 46 BF 7C 00 3D CC CC CD 3F 73 33 33 3F 80 00 00 A5 89` |
| 12:43:53.028 | CP AUTOADAPT -> PP, Hset 5.7 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 1 (PP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 01 7F FF FF FF E2 3E` |
| 12:44:00.951 | CP AUTOADAPT -> PP, Hset 5.7 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 1 (PP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 01 7F FF FF FF E2 3E` |
| 12:44:00.989 | CP AUTOADAPT -> PP, Hset 5.7 m | 86/18 type 302 v1 | feedback id 4, set point 78432 Pa (8.00 m), controller 0.5 / 8 / 0 | `27 1F E7 F8 0A 9B 56 00 12 01 2E 01 00 00 12 00 04 47 99 30 00 3F 00 00 00 41 00 00 00 00 00 00 00 99 5F` |
| 12:44:10.403 | CP AUTOADAPT -> PP, Hset 5.7 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 1 (PP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 01 7F FF FF FF E2 3E` |
| 12:44:10.437 | CP AUTOADAPT -> PP, Hset 5.7 m | 86/18 type 302 v1 | feedback id 4, set point 55882.4 Pa (5.70 m), controller 0.5 / 8 / 0 | `27 1F E7 F8 0A 9B 56 00 12 01 2E 01 00 00 12 00 04 47 5A 4A 5A 3F 00 00 00 41 00 00 00 00 00 00 00 A4 B7` |
| 12:44:50.600 | PP -> CP III (7.5 m) | 86/10 type 303 v1 | source 0, operation mode 6, control mode 0 (CP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 00 7F FF FF FF 48 6F` |
| 12:44:59.750 | PP -> CP III (7.5 m) | 86/10 type 303 v1 | source 0, operation mode 6, control mode 0 (CP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 00 7F FF FF FF 48 6F` |
| 12:44:59.791 | PP -> CP III (7.5 m) | 86/16 type 302 v1 | feedback id 4, set point 24510 Pa (2.50 m), controller 0.6 / 1.2 / 0 | `27 1F E7 F8 0A 9B 56 00 10 01 2E 01 00 00 12 00 04 46 BF 7C 00 3F 19 99 9A 3F 99 99 9A 00 00 00 00 CA AE` |
| 12:45:09.354 | PP -> CP III (7.5 m) | 86/10 type 303 v1 | source 0, operation mode 6, control mode 0 (CP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 00 7F FF FF FF 48 6F` |
| 12:45:09.388 | PP -> CP III (7.5 m) | 86/16 type 302 v1 | feedback id 4, set point 73529.4 Pa (7.50 m), controller 0.6 / 1.2 / 0 | `27 1F E7 F8 0A 9B 56 00 10 01 2E 01 00 00 12 00 04 47 8F 9C B5 3F 19 99 9A 3F 99 99 9A 00 00 00 00 9E A8` |
| 12:45:41.031 | PP -> CP III (7.5 m) | 86/10 type 303 v1 | source 0, operation mode 6, control mode 0 (CP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 00 7F FF FF FF 48 6F` |
| 12:45:41.057 | PP -> CP III (7.5 m) | 86/16 type 302 v1 | feedback id 4, set point 73529.4 Pa (7.50 m), controller 0.6 / 1.2 / 0 | `27 1F E7 F8 0A 9B 56 00 10 01 2E 01 00 00 12 00 04 47 8F 9C B5 3F 19 99 9A 3F 99 99 9A 00 00 00 00 9E A8` |
| 12:45:50.179 | CP III -> CP 4.0 m (Advanced) | 86/10 type 303 v1 | source 0, operation mode 6, control mode 0 (CP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 00 7F FF FF FF 48 6F` |
| 12:45:50.215 | CP III -> CP 4.0 m (Advanced) | 86/16 type 302 v1 | feedback id 4, set point 39215.7 Pa (4.00 m), controller 0.6 / 1.2 / 0 | `27 1F E7 F8 0A 9B 56 00 10 01 2E 01 00 00 12 00 04 47 19 2F B0 3F 19 99 9A 3F 99 99 9A 00 00 00 00 93 70` |
| 12:46:20.005 | CP 4.0 m -> CP AUTOADAPT | 86/10 type 303 v1 | source 0, operation mode 6, control mode 14 (CP AUTOADAPT), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 0E 7F FF FF FF 87 C7` |
| 12:46:20.049 | CP 4.0 m -> CP AUTOADAPT | 86/21 type 301 v1 | default 49020, min 24510, max 73530, resulting min 24510, controller 0.1 / 0.95 / 1 (written back unchanged) | `27 29 E7 F8 0A A5 56 00 15 01 2D 01 00 00 1C 47 3F 7C 00 46 BF 7C 00 47 8F 9D 00 46 BF 7C 00 3D CC CC CD 3F 73 33 33 3F 80 00 00 A5 89` |
| 12:46:44.338 | Duty point (flow and head) -> CP 4.0 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 0 (CP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 00 7F FF FF FF 48 6F` |
| 12:46:47.680 | Duty point (flow and head) -> CP 4.0 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 0 (CP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 00 7F FF FF FF 48 6F` |
| 12:46:47.713 | Duty point (flow and head) -> CP 4.0 m | 86/16 type 302 v1 | feedback id 4, set point 39215.7 Pa (4.00 m), controller 0.6 / 1.2 / 0 | `27 1F E7 F8 0A 9B 56 00 10 01 2E 01 00 00 12 00 04 47 19 2F B0 3F 19 99 9A 3F 99 99 9A 00 00 00 00 93 70` |

### Object values read

First value after connecting, then every change. 86/6 and 86/8 change all the time and are only listed once.

| Time | Object | Type | Data | Meaning |
| --- | --- | --- | --- | --- |
| 12:42:21.106 | 86/10 local control mode | 303 | `00 06 00 7F FF FF FF` | source 0, operation mode 6, control mode 0 (CP), set point NaN |
| 12:42:21.195 | 90/4 PP curve | 286 | `00 3F 00 00 00 00 00 00 00 3A D1 C1 52 B2 86 FE 96 28 FC 33 9B 3F 00 00 00` | quadratic 0, foot point 0.5, filter 0, coef 0.0016003 / -1.57154e-08 / 2.8e-14, foot point 0.5 |
| 12:42:21.285 | 86/6 local request | 303 | `00 00 00 46 BF 7C 00` | source 0, operation mode 0, control mode 0 (CP), set point 2.50 m |
| 12:42:21.374 | 86/39 CF limits | 301 | `3A 11 A2 B4 38 91 A2 B4 3A 8A 5A 91 38 91 A2 B4 4B 18 96 80 3F 73 33 33 3F 80 00 00` | default 0.000555556, min 6.94444e-05, max 0.00105556, resulting min 6.94444e-05, controller 1e+07 / 0.95 / 1 |
| 12:42:21.479 | 86/16 CP set point | 302 | `00 04 46 BF 7C 00 3F 19 99 9A 3F 99 99 9A 00 00 00 00` | feedback id 4, set point 24510 Pa (2.50 m), controller 0.6 / 1.2 / 0 |
| 12:42:21.534 | 86/600 max flow limiter | 895 | `01 00 3A 11 A5 AF 3F 00 00 00 40 00 00 00 00 00 00 00` | limiter 1, enable 0, limit 0.0005556 m3/s (2.00 m3/h), controller 0.5 / 2 / 0 |
| 12:42:21.622 | 86/14 CC set point | 302 | `00 FF 45 C6 70 00 3F 80 00 00 3F 80 00 00 3F 80 00 00` | feedback id 255, set point 6350, controller 1 / 1 / 1 |
| 12:42:21.667 | 86/18 PP set point | 302 | `00 04 47 99 30 00 3F 00 00 00 41 00 00 00 00 00 00 00` | feedback id 4, set point 78432 Pa (8.00 m), controller 0.5 / 8 / 0 |
| 12:42:21.712 | 86/40 CF set point | 302 | `00 06 3A 11 A2 B4 3F 19 99 9A 3F CC CC CD 3E CC CC CD` | feedback id 6, set point 0.000555556 m3/s (2.00 m3/h), controller 0.6 / 1.6 / 0.4 |
| 12:42:21.767 | 86/13 CC limits | 301 | `45 3B 80 00 44 CE 40 00 45 C6 70 00 C5 C6 70 00 3F 80 00 00 3F 80 00 00 3F 80 00 00` | default 3000, min 1650, max 6350, resulting min -6350, controller 1 / 1 / 1 |
| 12:42:21.815 | 86/17 PP limits | 301 | `47 08 B8 00 46 65 C8 00 47 A2 C3 00 46 19 30 00 3D CC CC CD 3F 73 33 33 3F 80 00 00` | default 35000, min 14706, max 83334, resulting min 9804, controller 0.1 / 0.95 / 1 |
| 12:42:21.859 | 86/15 CP limits | 301 | `47 1C 40 00 46 19 30 00 47 A2 C3 00 46 19 30 00 3D CC CC CD 3F 73 33 33 3F 80 00 00` | default 40000, min 9804, max 83334, resulting min 9804, controller 0.1 / 0.95 / 1 |
| 12:42:21.890 | 86/351 panel setting CC I | 869 | `02 45 A3 48 00` | control mode 2 (CC), set point 5225 RPM |
| 12:42:21.936 | 86/352 panel setting CC II | 869 | `02 45 B7 A8 00` | control mode 2 (CC), set point 5877 RPM |
| 12:42:21.970 | 86/353 panel setting CC III | 869 | `02 45 C6 70 00` | control mode 2 (CC), set point 6350 RPM |
| 12:42:22.014 | 86/354 panel setting CP I | 869 | `00 46 BF 7C 00` | control mode 0 (CP), set point 24510 Pa (2.50 m) |
| 12:42:22.059 | 86/355 panel setting CP II | 869 | `00 47 3F 7C 00` | control mode 0 (CP), set point 49020 Pa (5.00 m) |
| 12:42:22.104 | 86/356 panel setting CP III | 869 | `00 47 8F 9D 00` | control mode 0 (CP), set point 73530 Pa (7.50 m) |
| 12:42:22.150 | 86/357 panel setting PP I | 869 | `01 47 19 30 00` | control mode 1 (PP), set point 39216 Pa (4.00 m) |
| 12:42:22.194 | 86/358 panel setting PP II | 869 | `01 47 65 C8 00` | control mode 1 (PP), set point 58824 Pa (6.00 m) |
| 12:42:22.229 | 86/359 panel setting PP III | 869 | `01 47 99 30 00` | control mode 1 (PP), set point 78432 Pa (8.00 m) |
| 12:42:22.277 | 86/21 CP AUTOADAPT limits | 301 | `47 3F 7C 00 46 BF 7C 00 47 8F 9D 00 46 BF 7C 00 3D CC CC CD 3F 73 33 33 3F 80 00 00` | default 49020, min 24510, max 73530, resulting min 24510, controller 0.1 / 0.95 / 1 |
| 12:42:22.321 | 86/19 PP AUTOADAPT limits | 301 | `47 65 C8 00 47 19 30 00 47 99 30 00 46 19 30 00 3D CC CC CD 3F 73 33 33 3F 80 00 00` | default 58824, min 39216, max 78432, resulting min 9804, controller 0.1 / 0.95 / 1 |
| 12:42:22.455 | 86/8 realized request | 303 | `01 00 00 46 BF 7C 00` | source 1, operation mode 0, control mode 0 (CP), set point 2.50 m |
| 12:42:22.712 | 90/8 AutoNight | 333 | `00` | enabled 0 |
| 12:42:22.759 | 86/601 min flow limiter | 895 | `02 00 39 91 A5 AF 3F 00 00 00 40 00 00 00 00 00 00 00` | limiter 2, enable 0, limit 0.0002778 m3/s (1.00 m3/h), controller 0.5 / 2 / 0 |
| 12:42:23.444 | 93/200 QH coefficient | 563 | `3A 63 43 C4` | coefficient 0.000866946 m3/s |
| 12:42:23.490 | 93/184 QH coefficients 4 | 1114 | `C5 36 CE 82 3F 27 58 0B 3B 0D 3A BE 00 00 00 00` | coefficients -2924.91 / 0.653687 / 0.00215499 / 0 |
| 12:42:23.579 | 93/180 QH coefficients 0 | 1114 | `3E C2 6D 20 3A B6 8C 2C B4 C1 97 1A 2F 32 95 30` | coefficients 0.379739 / 0.00139273 / -3.6059e-07 / 1.6242e-10 |
| 12:42:23.625 | 93/181 QH coefficients 1 | 1114 | `3E 52 62 E5 BA 27 FE A4 35 8F 7C 34 AD 93 77 20` | coefficients 0.205455 / -0.000640849 / 1.06905e-06 / -1.67649e-11 |
| 12:42:23.670 | 93/182 QH coefficients 2 | 1114 | `3F 2E 03 D4 B9 7C D9 70 B3 E1 51 67 2D 7E 97 F6` | coefficients 0.679746 / -0.000241136 / -1.04922e-07 / 1.4472e-11 |
| 12:42:23.714 | 93/183 QH coefficients 3 | 1114 | `BF 67 96 89 39 AB CF DC B3 54 6B 3C 2C 0F 97 A5` | coefficients -0.904641 / 0.000327705 / -4.94576e-08 / 2.04057e-12 |
| 12:42:24.963 | 86/621 min flow limiter limits | 897 | `02 38 91 A2 B4 3A 11 A2 B4` | limiter 2, lower 6.94444e-05 (0.25 m3/h), upper 0.000555556 (2.00 m3/h) |
| 12:42:25.009 | 86/620 max flow limiter limits | 897 | `01 3A 11 A2 B4 3A 8A 5A 91` | limiter 1, lower 0.000555556 (2.00 m3/h), upper 0.00105556 (3.80 m3/h) |
| 12:43:00.358 | 86/10 local control mode | 303 | `00 06 0E 7F FF FF FF` | source 0, operation mode 6, control mode 14 (CP AUTOADAPT), set point NaN |
| 12:43:54.625 | 86/10 local control mode | 303 | `00 06 01 7F FF FF FF` | source 0, operation mode 6, control mode 1 (PP), set point NaN |
| 12:44:12.661 | 86/18 PP set point | 302 | `00 04 47 5A 4A 5A 3F 00 00 00 41 00 00 00 00 00 00 00` | feedback id 4, set point 55882.4 Pa (5.70 m), controller 0.5 / 8 / 0 |
| 12:44:51.177 | 86/10 local control mode | 303 | `00 06 00 7F FF FF FF` | source 0, operation mode 6, control mode 0 (CP), set point NaN |
| 12:45:11.553 | 86/16 CP set point | 302 | `00 04 47 8F 9C B5 3F 19 99 9A 3F 99 99 9A 00 00 00 00` | feedback id 4, set point 73529.4 Pa (7.50 m), controller 0.6 / 1.2 / 0 |
| 12:45:52.368 | 86/16 CP set point | 302 | `00 04 47 19 2F B0 3F 19 99 9A 3F 99 99 9A 00 00 00 00` | feedback id 4, set point 39215.7 Pa (4.00 m), controller 0.6 / 1.2 / 0 |
| 12:46:20.334 | 86/10 local control mode | 303 | `00 06 0E 7F FF FF FF` | source 0, operation mode 6, control mode 14 (CP AUTOADAPT), set point NaN |
| 12:46:45.943 | 86/10 local control mode | 303 | `00 06 00 7F FF FF FF` | source 0, operation mode 6, control mode 0 (CP), set point NaN |

### Connect sequence

Requests sent after connecting, before and between the class 10 reads.

| Request | Reply |
| --- | --- |
| class 2 GET `94 95 96` (`27 07 FF F8 02 03 94 95 96 4F 91`) | ACK 0, `34 06 00` |
| class 7 (ASCII) id 17 | "alpha2go" |
| class 7 (ASCII) id 1 | "ALPHA2 GO" |
| class 7 (ASCII) id 8 | "93074208" |
| class 7 (ASCII) id 9 | [serial number, left out] |
| class 7 (ASCII) id 10 | [production code, left out] |
| class 7 (ASCII) id 50 | "92975974V01.05.00.00888" |
| class 7 (ASCII) id 58 | "93127901V06.00.03.00001" |
| class 7 (ASCII) id 21 | "52_6_GSC_V03_93083569_20250403" |
| class 7 (ASCII) id 19 | "ALPHA2 GO XX-90" |
| class 5 GET `4B` (`27 05 E7 F8 05 01 4B D5 D6`) | ACK 0, `00` |
| class 11 GET `0F` (`27 05 E7 F8 0B 01 0F C6 97`) | ACK 0, `00 B8` |
| class 15 GET `1C` (`27 05 E7 F8 0F 01 1C 38 05`) | ACK 0, `00 00 00 02` |
| class 5 INFO `4B` (`27 05 E7 F8 05 C1 4B C3 82`) | ACK 0, `A1` |
| class 2 INFO `94 95 96` (`27 07 E7 F8 02 C3 94 95 96 58 E3`) | ACK 0, `80 80 80` |
| class 11 INFO `0F` (`27 05 E7 F8 0B C1 0F D0 C3`) | ACK 0, `80` |
| class 15 INFO `1C` (`27 05 E7 F8 0F C1 1C 2E 51`) | ACK 0, `80` |
| class 7 (ASCII) id 20 | "" |
| class 7 (ASCII) id 100 | [ECU id with serial number, left out] |
| class 2 GET `96 95 94` (`27 07 E7 F8 02 03 96 95 94 A5 65`) | ACK 0, `00 06 34` |
| class 2 INFO `96 95 94` (`27 07 E7 F8 02 C3 96 95 94 16 C1`) | ACK 0, `80 80 80` |

## Session 2

### Writes (class 10 SET)

Every write was answered with ACK OK (`24 .. 0A 01 00`).

| Time | Change made | Object | Value | Frame sent |
| --- | --- | --- | --- | --- |
| 13:46:42.121 | Phone connects | 202/9 type 292 v1 | ECU select = 0 | `27 11 E7 F8 0A 8D CA 00 09 01 24 01 00 00 04 00 00 00 00 50 1D` |
| 13:46:42.314 | Phone connects | 202/9 type 292 v1 | ECU select = 1 | `27 11 E7 F8 0A 8D CA 00 09 01 24 01 00 00 04 00 00 00 01 40 3C` |
| 13:47:05.668 | PP duty point 17 l/min at 5 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 1 (PP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 01 7F FF FF FF E2 3E` |
| 13:47:08.997 | PP duty point 17 l/min at 5 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 1 (PP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 01 7F FF FF FF E2 3E` |
| 13:47:09.032 | PP duty point 17 l/min at 5 m | 86/18 type 302 v1 | feedback id 4, set point 78432 Pa (8.00 m), controller 0.5 / 8 / 0 | `27 1F E7 F8 0A 9B 56 00 12 01 2E 01 00 00 12 00 04 47 99 30 00 3F 00 00 00 41 00 00 00 00 00 00 00 99 5F` |
| 13:47:34.982 | PP duty point 17 l/min at 5 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 1 (PP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 01 7F FF FF FF E2 3E` |
| 13:47:35.018 | PP duty point 17 l/min at 5 m | 86/18 type 302 v1 | feedback id 4, set point 82352.9 Pa (8.40 m), controller 0.5 / 8 / 0 | `27 1F E7 F8 0A 9B 56 00 12 01 2E 01 00 00 12 00 04 47 A0 D8 78 3F 00 00 00 41 00 00 00 00 00 00 00 A3 4B` |
| 13:48:14.676 | Constant flow | 86/10 type 303 v1 | source 0, operation mode 6, control mode 8 (CF), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 08 7F FF FF FF 4A 42` |
| 13:48:41.874 | Constant flow 1.0 m3/h | 86/10 type 303 v1 | source 0, operation mode 6, control mode 8 (CF), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 08 7F FF FF FF 4A 42` |
| 13:48:41.910 | Constant flow 1.0 m3/h | 86/40 type 302 v1 | feedback id 6, set point 0.000277778 m3/s (1.00 m3/h), controller 0.6 / 1.6 / 0.4 | `27 1F E7 F8 0A 9B 56 00 28 01 2E 01 00 00 12 00 06 39 91 A2 B4 3F 19 99 9A 3F CC CC CD 3E CC CC CD B3 87` |
| 13:48:58.141 | Constant curve | 86/10 type 303 v1 | source 0, operation mode 6, control mode 2 (CC), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 02 7F FF FF FF 0C EC` |
| 13:49:29.238 | Constant curve, speed in RPM | 86/10 type 303 v1 | source 0, operation mode 6, control mode 2 (CC), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 02 7F FF FF FF 0C EC` |
| 13:49:29.273 | Constant curve, speed in RPM | 86/14 type 302 v1 | feedback id 255, set point 5537.2, controller 1 / 1 / 1 | `27 1F E7 F8 0A 9B 56 00 0E 01 2E 01 00 00 12 00 FF 45 AD 09 9A 3F 80 00 00 3F 80 00 00 3F 80 00 00 BB 5D` |
| 13:49:44.463 | CP 4.0 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 0 (CP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 00 7F FF FF FF 48 6F` |
| 13:50:05.172 | CP 4.0 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 0 (CP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 00 7F FF FF FF 48 6F` |
| 13:50:05.207 | CP 4.0 m | 86/16 type 302 v1 | feedback id 4, set point 39215.7 Pa (4.00 m), controller 0.6 / 1.2 / 0 | `27 1F E7 F8 0A 9B 56 00 10 01 2E 01 00 00 12 00 04 47 19 2F B0 3F 19 99 9A 3F 99 99 9A 00 00 00 00 93 70` |
| 13:50:32.428 | CP duty point 17 l/min at 5 m | 86/10 type 303 v1 | source 0, operation mode 6, control mode 0 (CP), set point NaN | `27 14 E7 F8 0A 90 56 00 0A 01 2F 01 00 00 07 00 06 00 7F FF FF FF 48 6F` |
| 13:50:32.464 | CP duty point 17 l/min at 5 m | 86/16 type 302 v1 | feedback id 4, set point 49019.6 Pa (5.00 m), controller 0.6 / 1.2 / 0 | `27 1F E7 F8 0A 9B 56 00 10 01 2E 01 00 00 12 00 04 47 3F 7B 9C 3F 19 99 9A 3F 99 99 9A 00 00 00 00 EB 7D` |
| 13:51:11.701 | AutoNight on | 90/8 type 333 v1 | enabled 1 | `27 0E E7 F8 0A 8A 5A 00 08 01 4D 01 00 00 01 01 E1 DD` |
| 13:51:11.738 | AutoNight on | 90/8 type 333 v1 | enabled 1 | `27 0E E7 F8 0A 8A 5A 00 08 01 4D 01 00 00 01 01 E1 DD` |
| 13:51:35.550 | AutoNight off | 90/8 type 333 v1 | enabled 0 | `27 0E E7 F8 0A 8A 5A 00 08 01 4D 01 00 00 01 00 F1 FC` |
| 13:51:35.587 | AutoNight off | 90/8 type 333 v1 | enabled 0 | `27 0E E7 F8 0A 8A 5A 00 08 01 4D 01 00 00 01 00 F1 FC` |
| 13:52:29.411 | Max flow 3.0 m3/h | 86/600 type 895 v1 | limiter 1, enable 1, limit 0.0005556 m3/s (2.00 m3/h), controller 0.5 / 2 / 0 | `27 1F E7 F8 0A 9B 56 02 58 03 7F 01 00 00 12 01 01 3A 11 A5 AF 3F 00 00 00 40 00 00 00 00 00 00 00 CB C8` |
| 13:52:40.490 | Max flow 3.0 m3/h | 86/600 type 895 v1 | limiter 1, enable 1, limit 0.000833333 m3/s (3.00 m3/h), controller 0.5 / 2 / 0 | `27 1F E7 F8 0A 9B 56 02 58 03 7F 01 00 00 12 01 01 3A 5A 74 0E 3F 00 00 00 40 00 00 00 00 00 00 00 39 5D` |
| 13:52:55.893 | Min flow 0.7 m3/h | 86/601 type 895 v1 | limiter 2, enable 1, limit 0.0002778 m3/s (1.00 m3/h), controller 0.5 / 2 / 0 | `27 1F E7 F8 0A 9B 56 02 59 03 7F 01 00 00 12 02 01 39 91 A5 AF 3F 00 00 00 40 00 00 00 00 00 00 00 32 51` |
| 13:53:02.488 | Min flow 0.7 m3/h | 86/601 type 895 v1 | limiter 2, enable 1, limit 0.000194444 m3/s (0.70 m3/h), controller 0.5 / 2 / 0 | `27 1F E7 F8 0A 9B 56 02 59 03 7F 01 00 00 12 02 01 39 4B E3 C8 3F 00 00 00 40 00 00 00 00 00 00 00 F5 34` |
| 13:53:19.383 | Min flow off | 86/601 type 895 v1 | limiter 2, enable 0, limit 0.000194444 m3/s (0.70 m3/h), controller 0.5 / 2 / 0 | `27 1F E7 F8 0A 9B 56 02 59 03 7F 01 00 00 12 02 00 39 4B E3 C8 3F 00 00 00 40 00 00 00 00 00 00 00 E5 D6` |
| 13:53:45.749 | Max flow off | 86/600 type 895 v1 | limiter 1, enable 0, limit 0.000833333 m3/s (3.00 m3/h), controller 0.5 / 2 / 0 | `27 1F E7 F8 0A 9B 56 02 58 03 7F 01 00 00 12 01 00 3A 5A 74 0E 3F 00 00 00 40 00 00 00 00 00 00 00 29 BF` |

### Object values read

First value after connecting, then every change. 86/6 and 86/8 change all the time and are only listed once.

| Time | Object | Type | Data | Meaning |
| --- | --- | --- | --- | --- |
| 13:46:35.187 | 86/10 local control mode | 303 | `00 06 00 7F FF FF FF` | source 0, operation mode 6, control mode 0 (CP), set point NaN |
| 13:46:35.412 | 90/4 PP curve | 286 | `00 3F 00 00 00 00 00 00 00 3A D1 C1 52 B2 86 FE 96 28 FC 33 9B 3F 00 00 00` | quadratic 0, foot point 0.5, filter 0, coef 0.0016003 / -1.57154e-08 / 2.8e-14, foot point 0.5 |
| 13:46:35.595 | 86/6 local request | 303 | `00 00 00 47 19 2F B0` | source 0, operation mode 0, control mode 0 (CP), set point 4.00 m |
| 13:46:35.774 | 86/39 CF limits | 301 | `3A 11 A2 B4 38 91 A2 B4 3A 8A 5A 91 38 91 A2 B4 4B 18 96 80 3F 73 33 33 3F 80 00 00` | default 0.000555556, min 6.94444e-05, max 0.00105556, resulting min 6.94444e-05, controller 1e+07 / 0.95 / 1 |
| 13:46:35.955 | 86/16 CP set point | 302 | `00 04 47 19 2F B0 3F 19 99 9A 3F 99 99 9A 00 00 00 00` | feedback id 4, set point 39215.7 Pa (4.00 m), controller 0.6 / 1.2 / 0 |
| 13:46:36.028 | 86/600 max flow limiter | 895 | `01 00 3A 11 A5 AF 3F 00 00 00 40 00 00 00 00 00 00 00` | limiter 1, enable 0, limit 0.0005556 m3/s (2.00 m3/h), controller 0.5 / 2 / 0 |
| 13:46:36.119 | 86/14 CC set point | 302 | `00 FF 45 C6 70 00 3F 80 00 00 3F 80 00 00 3F 80 00 00` | feedback id 255, set point 6350, controller 1 / 1 / 1 |
| 13:46:36.163 | 86/18 PP set point | 302 | `00 04 47 99 30 00 3F 00 00 00 41 00 00 00 00 00 00 00` | feedback id 4, set point 78432 Pa (8.00 m), controller 0.5 / 8 / 0 |
| 13:46:36.209 | 86/40 CF set point | 302 | `00 06 3A 11 A2 B4 3F 19 99 9A 3F CC CC CD 3E CC CC CD` | feedback id 6, set point 0.000555556 m3/s (2.00 m3/h), controller 0.6 / 1.6 / 0.4 |
| 13:46:36.267 | 86/13 CC limits | 301 | `45 3B 80 00 44 CE 40 00 45 C6 70 00 C5 C6 70 00 3F 80 00 00 3F 80 00 00 3F 80 00 00` | default 3000, min 1650, max 6350, resulting min -6350, controller 1 / 1 / 1 |
| 13:46:36.310 | 86/17 PP limits | 301 | `47 08 B8 00 46 65 C8 00 47 A2 C3 00 46 19 30 00 3D CC CC CD 3F 73 33 33 3F 80 00 00` | default 35000, min 14706, max 83334, resulting min 9804, controller 0.1 / 0.95 / 1 |
| 13:46:36.354 | 86/15 CP limits | 301 | `47 1C 40 00 46 19 30 00 47 A2 C3 00 46 19 30 00 3D CC CC CD 3F 73 33 33 3F 80 00 00` | default 40000, min 9804, max 83334, resulting min 9804, controller 0.1 / 0.95 / 1 |
| 13:46:36.386 | 86/351 panel setting CC I | 869 | `02 45 A3 48 00` | control mode 2 (CC), set point 5225 RPM |
| 13:46:36.433 | 86/352 panel setting CC II | 869 | `02 45 B7 A8 00` | control mode 2 (CC), set point 5877 RPM |
| 13:46:36.478 | 86/353 panel setting CC III | 869 | `02 45 C6 70 00` | control mode 2 (CC), set point 6350 RPM |
| 13:46:36.522 | 86/354 panel setting CP I | 869 | `00 46 BF 7C 00` | control mode 0 (CP), set point 24510 Pa (2.50 m) |
| 13:46:36.556 | 86/355 panel setting CP II | 869 | `00 47 3F 7C 00` | control mode 0 (CP), set point 49020 Pa (5.00 m) |
| 13:46:36.602 | 86/356 panel setting CP III | 869 | `00 47 8F 9D 00` | control mode 0 (CP), set point 73530 Pa (7.50 m) |
| 13:46:36.646 | 86/357 panel setting PP I | 869 | `01 47 19 30 00` | control mode 1 (PP), set point 39216 Pa (4.00 m) |
| 13:46:36.692 | 86/358 panel setting PP II | 869 | `01 47 65 C8 00` | control mode 1 (PP), set point 58824 Pa (6.00 m) |
| 13:46:36.736 | 86/359 panel setting PP III | 869 | `01 47 99 30 00` | control mode 1 (PP), set point 78432 Pa (8.00 m) |
| 13:46:36.785 | 86/21 CP AUTOADAPT limits | 301 | `47 3F 7C 00 46 BF 7C 00 47 8F 9D 00 46 BF 7C 00 3D CC CC CD 3F 73 33 33 3F 80 00 00` | default 49020, min 24510, max 73530, resulting min 24510, controller 0.1 / 0.95 / 1 |
| 13:46:36.829 | 86/19 PP AUTOADAPT limits | 301 | `47 65 C8 00 47 19 30 00 47 99 30 00 46 19 30 00 3D CC CC CD 3F 73 33 33 3F 80 00 00` | default 58824, min 39216, max 78432, resulting min 9804, controller 0.1 / 0.95 / 1 |
| 13:46:36.940 | 86/8 realized request | 303 | `01 00 00 47 19 2F B0` | source 1, operation mode 0, control mode 0 (CP), set point 4.00 m |
| 13:46:37.208 | 90/8 AutoNight | 333 | `00` | enabled 0 |
| 13:46:37.254 | 86/601 min flow limiter | 895 | `02 00 39 91 A5 AF 3F 00 00 00 40 00 00 00 00 00 00 00` | limiter 2, enable 0, limit 0.0002778 m3/s (1.00 m3/h), controller 0.5 / 2 / 0 |
| 13:46:37.950 | 93/200 QH coefficient | 563 | `3A 63 43 C4` | coefficient 0.000866946 m3/s |
| 13:46:37.985 | 93/184 QH coefficients 4 | 1114 | `C5 36 CE 82 3F 27 58 0B 3B 0D 3A BE 00 00 00 00` | coefficients -2924.91 / 0.653687 / 0.00215499 / 0 |
| 13:46:38.075 | 93/180 QH coefficients 0 | 1114 | `3E C2 6D 20 3A B6 8C 2C B4 C1 97 1A 2F 32 95 30` | coefficients 0.379739 / 0.00139273 / -3.6059e-07 / 1.6242e-10 |
| 13:46:38.121 | 93/181 QH coefficients 1 | 1114 | `3E 52 62 E5 BA 27 FE A4 35 8F 7C 34 AD 93 77 20` | coefficients 0.205455 / -0.000640849 / 1.06905e-06 / -1.67649e-11 |
| 13:46:38.164 | 93/182 QH coefficients 2 | 1114 | `3F 2E 03 D4 B9 7C D9 70 B3 E1 51 67 2D 7E 97 F6` | coefficients 0.679746 / -0.000241136 / -1.04922e-07 / 1.4472e-11 |
| 13:46:38.210 | 93/183 QH coefficients 3 | 1114 | `BF 67 96 89 39 AB CF DC B3 54 6B 3C 2C 0F 97 A5` | coefficients -0.904641 / 0.000327705 / -4.94576e-08 / 2.04057e-12 |
| 13:46:39.480 | 86/621 min flow limiter limits | 897 | `02 38 91 A2 B4 3A 11 A2 B4` | limiter 2, lower 6.94444e-05 (0.25 m3/h), upper 0.000555556 (2.00 m3/h) |
| 13:46:39.515 | 86/620 max flow limiter limits | 897 | `01 3A 11 A2 B4 3A 8A 5A 91` | limiter 1, lower 0.000555556 (2.00 m3/h), upper 0.00105556 (3.80 m3/h) |
| 13:47:07.268 | 86/10 local control mode | 303 | `00 06 01 7F FF FF FF` | source 0, operation mode 6, control mode 1 (PP), set point NaN |
| 13:47:37.197 | 86/18 PP set point | 302 | `00 04 47 A0 D8 78 3F 00 00 00 41 00 00 00 00 00 00 00` | feedback id 4, set point 82352.9 Pa (8.40 m), controller 0.5 / 8 / 0 |
| 13:48:15.645 | 86/10 local control mode | 303 | `00 06 08 7F FF FF FF` | source 0, operation mode 6, control mode 8 (CF), set point NaN |
| 13:48:44.189 | 86/40 CF set point | 302 | `00 06 39 91 A2 B4 3F 19 99 9A 3F CC CC CD 3E CC CC CD` | feedback id 6, set point 0.000277778 m3/s (1.00 m3/h), controller 0.6 / 1.6 / 0.4 |
| 13:48:58.733 | 86/10 local control mode | 303 | `00 06 02 7F FF FF FF` | source 0, operation mode 6, control mode 2 (CC), set point NaN |
| 13:49:31.369 | 86/14 CC set point | 302 | `00 FF 45 AD 09 9A 3F 80 00 00 3F 80 00 00 3F 80 00 00` | feedback id 255, set point 5537.2, controller 1 / 1 / 1 |
| 13:49:46.064 | 86/10 local control mode | 303 | `00 06 00 7F FF FF FF` | source 0, operation mode 6, control mode 0 (CP), set point NaN |
| 13:50:34.628 | 86/16 CP set point | 302 | `00 04 47 3F 7B 9C 3F 19 99 9A 3F 99 99 9A 00 00 00 00` | feedback id 4, set point 49019.6 Pa (5.00 m), controller 0.6 / 1.2 / 0 |
| 13:51:16.164 | 90/8 AutoNight | 333 | `01` | enabled 1 |
| 13:51:42.905 | 90/8 AutoNight | 333 | `00` | enabled 0 |
| 13:52:42.250 | 86/600 max flow limiter | 895 | `01 01 3A 5A 74 0E 3F 00 00 00 40 00 00 00 00 00 00 00` | limiter 1, enable 1, limit 0.000833333 m3/s (3.00 m3/h), controller 0.5 / 2 / 0 |
| 13:53:04.143 | 86/601 min flow limiter | 895 | `02 01 39 4B E3 C8 3F 00 00 00 40 00 00 00 00 00 00 00` | limiter 2, enable 1, limit 0.000194444 m3/s (0.70 m3/h), controller 0.5 / 2 / 0 |
| 13:53:27.081 | 86/601 min flow limiter | 895 | `02 00 39 4B E3 C8 3F 00 00 00 40 00 00 00 00 00 00 00` | limiter 2, enable 0, limit 0.000194444 m3/s (0.70 m3/h), controller 0.5 / 2 / 0 |
| 13:53:57.938 | 86/600 max flow limiter | 895 | `01 00 3A 5A 74 0E 3F 00 00 00 40 00 00 00 00 00 00 00` | limiter 1, enable 0, limit 0.000833333 m3/s (3.00 m3/h), controller 0.5 / 2 / 0 |

