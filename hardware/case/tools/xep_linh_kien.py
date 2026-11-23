"""Xếp linh kiện (khối đơn giản theo kích thước datasheet) vào vỏ robot.
Chạy: pip install cadquery matplotlib && python hardware/case/tools/xep_linh_kien.py
Trục: x = trái(-)/phải(+), y = sau(-)/trước(+), z = lên. Đơn vị mm.
"""
import sys, itertools, pathlib
import cadquery as cq

HERE = pathlib.Path(__file__).resolve().parent.parent
CASE = HERE / "robot_vo_lap_rap.step"
OUT = HERE / "robot_lap_linh_kien.step"
CASE_NAMES = ["1_than", "2_nap", "3_co", "5_dau_duoi", "4_dau_tren", "6_nap_tan_sang_LED"]  # theo zmin

def box(x0, x1, y0, y1, z0, z1):
    return cq.Workplane().box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0)).val()

def cyl(r, h, p, d):  # bán kính, dài, điểm đáy, hướng trục
    return cq.Solid.makeCylinder(r, h, cq.Vector(*p), cq.Vector(*d))

def uni(*s):
    r = s[0]
    for o in s[1:]: r = r.fuse(o)
    return r.clean()

F = 43.5  # mặt trên sàn thân
P = {}  # tên -> (solid, màu, kích thước datasheet)

# ---- Tầng dưới (trong thân) ----
P["pin_18650_x2_chong_doc"] = (uni(box(-10.3, 8.7, -38.5, 38.5, F, F + 38)), "red", "2× 18650 Ø18.5×65 + đế chồng 19×77×38")
P["BMS_2S"] = (box(-13.8, -10.3, -22.5, 22.5, F + 3, F + 19), "purple", "45×16×3.5")
P["sac_IP2326"] = (uni(box(-36.8, -19.2, -33.25, -7.75, 49.9, 51.5),
                       box(-38.5, -31, -25, -16, 51.5, 54.8)), "purple", "25.5×17.6×5, cổng USB-C ra lỗ hông trái")
P["ha_ap_MP1584"] = (box(-36.8, -19.8, 0, 22, F + 0.5, F + 4.5), "purple", "22×17×4")
P["ESP32_S3_N16R8"] = (uni(box(8.8, 34.2, -47, 22, 48.3, 49.9),
                           box(12.5, 30.5, -1.5, 22, 49.9, 53.1),
                           box(17.7, 26.3, -48, -41, 49.9, 53.1)), "blue", "DevKitC-1 69×25.4, USB-C ra lỗ sau")
# ---- Tầng giữa ----
P["TB6612"] = (box(12, 32.3, -22, -1.7, 60, 65), "green", "20.3×20.3×5")
P["MPU6050"] = (box(12, 33, 3, 19, 60, 63), "green", "GY-521 21×16×3, nằm phẳng")
P["microSD"] = (box(14, 35, -40, -22, 68, 71.5), "green", "21×18×3.5")
P["INMP441"] = (box(-34, -20, 30, 44, 70, 73), "orange", "14×14×3, xa động cơ/loa")
P["DS3231"] = (box(-36.8, -27.8, -12, 26, 55, 77), "green", "ZS-042 38×22×9 (dựng đứng sát hông trái)")
P["MAX98357A"] = (box(-36.5, -18.7, -40, -20.6, 60, 63), "orange", "17.8×19.4×3")
P["loa_3525"] = (box(-17.5, 17.5, -48, -42, 56, 81), "gray", "35×25×6, áp lưới loa sau")
# ---- Mặt trước ----
P["VL53L0X"] = (uni(box(-13.5, 11.5, 44.5, 46, 52, 62.7), box(-2, 2, 46, 47.5, 55.5, 58.5)), "cyan", "GY-530 25×10.7, sau cửa cảm biến")
P["LD2410C"] = (box(-12, 10, 44.5, 47.5, 65.5, 81.5), "cyan", "22×16×3, sau vách trước (radar xuyên nhựa)")
# ---- Cổ / đầu ----
sg = uni(box(-6.1, 6.1, 4.45, 26.95, 82.7, 105.4), box(-6.1, 6.1, -0.4, 31.8, 98.6, 101.1),
         cyl(5.9, 1.2, (0, 21.6, 105.4), (0, 0, 1)), cyl(2.4, 3.2, (0, 21.6, 106.6), (0, 0, 1)))
P["servo_SG90"] = (sg, "darkgreen", "22.5×12.2×22.7, tai 32.2")
# kính 27.8×27.8×2.2 phía trước PCB 31.5×39.3×1.6 (chân ở dưới). Chỉ lọt ở tâm kính z=136, mặt kính y=38.
_yg, _zg = 38.0, 136.0
P["ST7789_1in54"] = (uni(box(-13.9, 13.9, _yg - 2.2, _yg, _zg - 13.9, _zg + 13.9),
                         box(-15.75, 15.75, _yg - 3.8, _yg - 2.2, _zg - 23.15, _zg + 16.15)), "black", "PCB 31.5×39.3 + kính 27.8×27.8")
P["WS2812_vong12"] = (cq.Solid.makeCylinder(16, 3.2, cq.Vector(0, 15, 148)).cut(cq.Solid.makeCylinder(9, 3.2, cq.Vector(0, 15, 148))), "white", "vòng 8 LED Ø32/Ø18×3.2 (vòng 12 LED Ø37 vướng màn hình)")
P["BH1750"] = (box(-9.25, 9.25, 8.05, 21.95, 144.5, 147.5), "green", "GY-302 18.5×13.9×3, dưới lỗ vòng LED")
# ---- Gầm + cụm bánh xích (ngoài vỏ) ----
# Mỗi bên: bánh chủ động (sau, gắn N20) + bánh dẫn hướng (trước) Ø33, 2 bánh đỡ Ø20, 1 con lăn trên Ø10,
# dải xích dày 2.5 rộng 16 + gân bám 1.5. Tổng xe ~120 (dài) × 112 (rộng) mm, đáy gân ở z = 0.
import math
YW, ZW = 39.0, 21.0            # tâm bánh chủ động/dẫn hướng: y = ∓YW, z = ZW
RI, TB, TG = 17.0, 2.5, 1.5    # bán kính trong dải xích, độ dày dải, cao gân
XT0, XT1 = 40.0, 56.0          # dải xích theo x (bên phải; bên trái lấy đối xứng)
XP0, XP1 = 36.0, 39.0          # tấm hông khung gầm
YR, ZR, RR = 11.0, ZW - RI + 10, 10.0   # bánh đỡ: y = ±YR, bán kính 10, tì mặt trong dải dưới
ZT, RT = ZW + RI - 5, 5.0               # con lăn trên ở y = 0

def mx(sx, x0, x1):  # khoảng x theo bên
    return (x0, x1) if sx > 0 else (-x1, -x0)

def cx(r, sx, x0, x1, y, z):  # trụ trục x
    a, b = mx(sx, x0, x1)
    return cyl(r, b - a, (a, y, z), (1, 0, 0))

def stadium(sx, r):
    a, b = mx(sx, XT0, XT1)
    return uni(box(a, b, -YW, YW, ZW - r, ZW + r), cyl(r, b - a, (a, -YW, ZW), (1, 0, 0)), cyl(r, b - a, (a, YW, ZW), (1, 0, 0)))

def track_band(sx):
    band = stadium(sx, RI + TB).cut(stadium(sx, RI))
    a, b = mx(sx, XT0, XT1)
    L = 4 * YW + 2 * math.pi * (RI + TB); n = 40; ro = RI + TB
    g = []
    for i in range(n):
        t = i * L / n
        if t < 2 * YW: y, z, ang = -YW + t, ZW - ro, 0                    # đoạn dưới, đi về trước
        elif t < 2 * YW + math.pi * ro:
            ang = (t - 2 * YW) / ro; y, z = YW + ro * math.sin(ang), ZW - ro * math.cos(ang)
            ang = math.degrees(ang)
        elif t < 4 * YW + math.pi * ro: y, z, ang = YW - (t - 2 * YW - math.pi * ro), ZW + ro, 180
        else:
            ang = (t - 4 * YW - math.pi * ro) / ro; y, z = -YW - ro * math.sin(ang), ZW + ro * math.cos(ang)
            ang = 180 + math.degrees(ang)
        gb = cq.Workplane().box(b - a, 2.0, TG + 0.2).translate(((a + b) / 2, 0, -(TG / 2) + 0.1)).val()
        g.append(gb.rotate(cq.Vector(0, 0, 0), cq.Vector(1, 0, 0), ang).translate(cq.Vector(0, y, z)))
    return uni(band, *g)

def sprocket(sx):  # bánh chủ động: thân Ø30 + 12 răng tới Ø33, moay-ơ Ø10 thò vào trong tới tấm hông
    body = cx(15, sx, 41, 55, -YW, ZW)
    teeth = [cq.Workplane().box(14, 3, 3).translate(((48 if sx > 0 else -48), 0, 15)).val()
             .rotate(cq.Vector(0, 0, 0), cq.Vector(1, 0, 0), k * 30).translate(cq.Vector(0, -YW, ZW)) for k in range(12)]
    return uni(body, cx(5, sx, 39.5, 41, -YW, ZW), *teeth).cut(cx(1.5, sx, 39.5, 47, -YW, ZW))  # lỗ trục sâu 7.5

def idler(sx):  # bánh dẫn hướng: Ø33 có 2 vành, giữa lõm Ø30 để gân dẫn của xích chạy
    return uni(cx(16.5, sx, 41, 43, YW, ZW), cx(15, sx, 43, 53, YW, ZW), cx(16.5, sx, 53, 55, YW, ZW), cx(5, sx, 39.5, 41, YW, ZW))

def road_wheels(sx):
    return uni(*[cx(RR, sx, 41, 55, y, ZR) for y in (-YR, YR)], *[cx(3, sx, 39.5, 41, y, ZR) for y in (-YR, YR)])

def top_roller(sx):
    return uni(cx(RT, sx, 41, 55, 0, ZT), cx(2.5, sx, 39.5, 41, 0, ZT))

def n20(sx):  # N20: hộp số 12×10×9 + motor 12×10×15, trục Ø3 dài 10 xuyên tấm hông vào moay-ơ bánh chủ động
    a, b = mx(sx, 12, 36)
    return uni(box(a, b, -YW - 6, -YW + 6, ZW - 5, ZW + 5), cx(1.5, sx, 36, 46, -YW, ZW))

def side_plate(sx):  # tấm hông 3 mm giữ trục các bánh, lỗ trục bánh chủ động Ø4
    a, b = mx(sx, XP0, XP1)
    pl = box(a, b, -46, 46, 8, 36).cut(cx(2, sx, XP0, XP1, -YW, ZW))
    return pl

khung = uni(box(-39, 39, -46, 46, 33, 36), side_plate(-1), side_plate(1),
            *[cyl(2.5, 5, (sx * 31, sy * 42, 36), (0, 0, 1)) for sx in (-1, 1) for sy in (-1, 1)])
P["khung_gam"] = (khung, "silver", "tấm gầm 78×92×3 + 2 tấm hông 3×92×28 + 4 trụ M3 vào lỗ sàn")
for sx, ten in ((-1, "trai"), (1, "phai")):
    P[f"N20_{ten}"] = (n20(sx), "gold", "N20 12×10×24, trục Ø3×10 ra bánh chủ động")
    P[f"banh_chu_dong_{ten}"] = (sprocket(sx), "darkorange", "Ø33 × 14, 12 răng, moay-ơ lỗ D 3 mm")
    P[f"banh_dan_huong_{ten}"] = (idler(sx), "orange", "Ø33 × 14, 2 vành, trục M3")
    P[f"banh_do_{ten}"] = (road_wheels(sx), "peru", "2× Ø20 × 14, trục M3")
    P[f"con_lan_tren_{ten}"] = (top_roller(sx), "peru", "Ø10 × 14, đỡ dải xích trên")
    P[f"xich_{ten}"] = (track_band(sx), "dimgray", "dải dày 2.5 rộng 16, 40 gân bám 1.5 (TPU)")
P["TCRT5000_trai"] = (box(-16, -6, 40, 46, 4, 11), "cyan", "10×6×7 trên giá mũi, cách đất ~4 mm")
P["TCRT5000_phai"] = (box(6, 16, 40, 46, 4, 11), "cyan", "10×6×7 trên giá mũi, cách đất ~4 mm")

def main():
    a = cq.importers.importStep(str(CASE))
    case = dict(zip(CASE_NAMES, sorted(a.solids().vals(), key=lambda s: s.BoundingBox().zmin)))
    vol = lambda s1, s2: s1.intersect(s2).Volume()
    bad = []
    for n, (s, _, _) in P.items():
        for cn, c in case.items():
            v = vol(s, c)
            if v > 1e-3: bad.append(f"{n} chạm {cn}: {v:.1f} mm³")
    for (n1, (s1, _, _)), (n2, (s2, _, _)) in itertools.combinations(P.items(), 2):
        v = vol(s1, s2)
        if v > 1e-3: bad.append(f"{n1} chạm {n2}: {v:.1f} mm³")
    print("\n".join(bad) or "Không có va chạm.")
    asm = cq.Assembly(name="robot_lap_linh_kien")
    for cn, c in case.items(): asm.add(c, name=cn, color=cq.Color(0.85, 0.85, 0.8, 0.35))
    for n, (s, col, _) in P.items(): asm.add(s, name=n, color=cq.Color(*__import__("matplotlib.colors").colors.to_rgb(col)))
    asm.save(str(OUT)); print("Đã ghi", OUT)
    for n, (s, _, d) in P.items():
        b = s.BoundingBox(); print(f"| {n} | {d} | x {b.xmin:.0f}…{b.xmax:.0f}, y {b.ymin:.0f}…{b.ymax:.0f}, z {b.zmin:.0f}…{b.zmax:.0f} |")
    if "--png" in sys.argv: render(case)

def render(case):
    import matplotlib; matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection
    fig = plt.figure(figsize=(14, 7), dpi=110)
    for i, (el, az) in enumerate([(22, -60), (22, 120)]):
        ax = fig.add_subplot(1, 2, i + 1, projection="3d")
        items = [(c, (0.8, 0.8, 0.75, 0.08)) for c in case.values()] + [(s, matplotlib.colors.to_rgba(col, 0.95)) for s, col, _ in P.values()]
        for s, col in items:
            v, t = s.tessellate(0.5, 0.5)
            tri = [[(v[k].x, v[k].y, v[k].z) for k in f] for f in t]
            ax.add_collection3d(Poly3DCollection(tri, facecolor=col, edgecolor=(0, 0, 0, 0.02 if col[3] < 0.5 else 0.15), linewidths=0.2))
        ax.set_xlim(-65, 65); ax.set_ylim(-65, 65); ax.set_zlim(0, 160); ax.set_box_aspect((130, 130, 160))
        ax.view_init(el, az); ax.set_axis_off()
        ax.set_title("nhìn phía sau" if i == 0 else "nhìn phía trước")
    plt.tight_layout(); plt.savefig(HERE / "robot_lap_linh_kien.png"); print("Đã ghi PNG")

if __name__ == "__main__":
    main()
