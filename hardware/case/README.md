# Vỏ robot (3D)

- `robot_vo_lap_rap.step`: bản lắp ráp vỏ robot (STEP AP214).

## Mở file
- **Onshape**: Documents → Create → Import → chọn file `.step` (tick "Import as assembly" nếu muốn giữ cấu trúc lắp ráp).
- **Fusion 360**: File → Open → Open from my computer → chọn file `.step`.
- **FreeCAD** (miễn phí, offline): File → Open.

Khi sửa xong, xuất lại STEP và ghi đè file này (đặt tên giữ nguyên để lịch sử git theo dõi được).

## Kích thước (cập nhật 22/11/2026: rút ngắn cho gần vuông)
| | Trước | Sau |
|---|---|---|
| Thân + nắp (R × D) | 78 × 140 mm | 78 × 100 mm |
| Chiều cao vỏ (đáy thân → đỉnh đầu) | 116.6 mm | 116.6 mm (giữ nguyên) |
| Lòng thân (R × D × C) | ~74 × 136 × 40 mm | ~74 × 96 × 40 mm |

Cách sửa: cắt bỏ 2 lát không có chi tiết trên thân và nắp (14 mm phía trước, 26 mm phía sau),
nên lỗ loa, cửa cảm biến, lỗ cổng, cột vít, ô servo SG90 và lỗ cổ giữ nguyên kích thước.
Cổ và đầu dời theo để vẫn nằm đúng trên ô servo.

## Xếp linh kiện trong vỏ (22/11/2026)
- `robot_lap_linh_kien.step`: vỏ (trong suốt) + linh kiện dạng khối theo kích thước datasheet, mỗi linh kiện là 1 body có tên.
- `robot_lap_linh_kien.png`: ảnh xem trước. `tools/xep_linh_kien.py`: script CadQuery tạo lại file và kiểm tra va chạm
  (`pip install cadquery matplotlib && python hardware/case/tools/xep_linh_kien.py --png`).

Trục: x trái(−)/phải(+), y sau(−)/trước(+), z lên; đáy xích ở z = 0, sàn thân z = 41. Mặt trước là vách có cửa cảm biến, mặt sau có lưới loa và lỗ USB.

| Linh kiện | Kích thước (mm) | Vị trí |
|---|---|---|
| 2× 18650 + đế | 19 × 77 × 38, **chồng 2 tầng** | giữa thân, trên sàn |
| BMS 2S | 45 × 16 × 3.5 | dựng đứng sát hông trái của pin |
| Sạc IP2326 | 25.5 × 17.6 × 5 | góc sau-trái, USB-C ra lỗ hông trái |
| Hạ áp MP1584 | 22 × 17 × 4 | góc trước-trái, nằm sàn |
| ESP32-S3 N16R8 DevKitC-1 | 69 × 25.4 × 5 | nằm sàn bên phải, USB-C ra lỗ sau |
| TB6612 | 20.3 × 20.3 × 5 | phải, tầng giữa, gần lỗ dây sàn xuống động cơ |
| MPU6050 (GY-521) | 21 × 16 × 3 | phải, tầng giữa, nằm phẳng gần tâm |
| microSD | 21 × 18 × 3.5 | phải-sau, tầng trên |
| DS3231 (ZS-042) | 38 × 22 × 9 | dựng đứng sát hông trái |
| MAX98357A | 17.8 × 19.4 × 3 | trái-sau, trên mạch sạc |
| Loa 3525 | 35 × 25 × 6 | áp lưới loa mặt sau |
| INMP441 | 14 × 14 × 3 | trước-trái tầng trên (xa động cơ và loa) |
| VL53L0X (GY-530) | 25 × 10.7 | sau cửa cảm biến mặt trước |
| LD2410C | 22 × 16 × 3 | trên cửa cảm biến, sau vách trước (radar xuyên nhựa) |
| SG90 | 22.5 × 12.2 × 22.7, tai 32.2 | ô servo ở nắp, tai đặt trên mặt nắp |
| ST7789 1.54" | PCB 31.5 × 39.3 + kính 27.8 | trong đầu, mặt kính cách cửa mặt ~4 mm |
| Vòng WS2812 **8 LED** | Ø32 / Ø18 | dưới nắp tản sáng đỉnh đầu |
| BH1750 (GY-302) | 18.5 × 13.9 × 3 | ngay dưới lỗ giữa vòng LED |
| Khung gầm | tấm 78 × 92 × 3 + 2 tấm hông 3 × 92 × 28 | dưới thân, bắt 4 vít vào lỗ sàn; tấm hông giữ trục bánh |
| 2× N20 | 12 × 10 × 24, trục Ø3 × 10 | trong gầm phía sau, nằm ngang, trục xuyên tấm hông vào bánh chủ động |
| 2× TCRT5000 | 10 × 6 × 7 | giá dưới mũi, cách đất ~4 mm |

### Chỗ chưa ổn
1. **SG90 chạm đĩa cổ** (`3_co`) khoảng 4 mm: tai servo đặt trên mặt nắp (z 98.6) thì đỉnh thân servo lên tới z 105.4, mà đáy đĩa cổ ở z 101.5. Cách sửa: nâng cổ + đầu lên ~4.5 mm, hoặc khoét lõm đáy đĩa cổ 12.5 × 23 × 4.5 mm.
2. **Pin phải chồng 2 tầng**: nằm cạnh nhau thì chiếm hết sàn, ESP32 không xuống được ngang lỗ USB sau (z 49–54) và mạch sạc không với tới lỗ hông trái. Đỉnh pin cách đáy servo chỉ ~1 mm.
3. **ESP32 phải nằm sát sàn** (chỉ 4.8 mm dưới mạch): không hàn header, hàn dây thẳng hoặc dùng header góc.
4. **Vòng 12 LED (Ø37) không vừa** vì chạm mép trên màn hình; dùng vòng 8 LED Ø32. BH1750 nằm dưới vòng nên đọc sáng lúc LED tắt.
5. **Màn hình** chỉ lọt ở đúng độ cao giữa đầu, kính lùi sau cửa mặt ~4 mm: nên in thêm khung đệm.
6. **TCRT5000** không đặt dưới sàn thân được (cách đất 41 mm, quá xa); cần giá gắn ở mũi khung gầm, cách đất 2–8 mm.
7. Khe nhỏ dưới cửa cảm biến (10 × 3 mm, z 47–50) chưa dùng.

## Cụm bánh xích (23/11/2026)
Mỗi bên (đối xứng trái/phải), nằm ngoài vỏ, x = ±40…56:

| Chi tiết | Kích thước (mm) | Vị trí tâm (y, z) |
|---|---|---|
| Bánh chủ động (sau) | Ø33 × 14, 12 răng, moay-ơ Ø10, lỗ trục D 3 sâu 7.5 | (−39, 21), gắn thẳng trục N20 |
| Bánh dẫn hướng (trước) | Ø33 × 14, 2 vành Ø33, giữa Ø30 | (+39, 21), trục M3 |
| 2 bánh đỡ | Ø20 × 14 | (±11, 14), trục M3 |
| Con lăn trên | Ø10 × 14 | (0, 33), đỡ dải xích trên |
| Dải xích | dày 2.5, rộng 16, 40 gân bám cao 1.5 (in TPU 95A) | ôm bánh chủ động + dẫn hướng |

- **Kích thước cả xe: 120 (dài) × 112 (rộng) × 158 (cao) mm**, khoảng sáng gầm ~8 mm dưới tấm hông.
- Kiểm tra va chạm: các bánh, dải xích, N20, khung gầm không chạm nhau và không chạm vỏ (chỉ còn lỗi SG90 ↔ đĩa cổ ở trên).
- Chu vi dải xích (mặt trung bình) ≈ 4 × 39 + 2π × 18.25 ≈ 271 mm. Nên in dải dài hơn ~1–2 mm rồi chỉnh bằng lỗ trục bánh dẫn hướng dạng rãnh (căng xích).
- Bánh chủ động Ø33 với N20 ~300 v/ph cho tốc độ ~0.5 m/s.
