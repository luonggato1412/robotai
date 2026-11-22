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
