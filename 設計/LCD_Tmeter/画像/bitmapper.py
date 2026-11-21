#------------------------------------------------------
# ①　PNGを開く
# ②　96 x 96にリサイズ
# ③　各ピクセル値を取得
# ④　RGB888(各色8ビット) →　RGB565へ変換
# ⑤　C配列として書き出し
#------------------------------------------------------

# ①　画像を開く
from PIL import Image
img = Image.open("hum.png").convert("RGB")

x_size = 70
y_size = 70

# ②　リサイズ
img = img.resize((x_size, y_size))

# ③　ピクセル走査
data = []
for y in range(y_size):
    for x in range(x_size):
        r, g, b = img.getpixel((x, y))

        # ④　RGB888 → RGB565変換
        # R = r & 1111 1000 (上位5ビット残し) << 8 (16ビット位置へ移動)
        # G = g & 1111 1100 (上位6ビット残し) << 3 (移動)
        # B = b << 3 (下位3ビット削る)
        # RRRRR GGGGGG BBBBB
        rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        data.append(rgb565)

with open("hum.h", "w") as f:
    # ヘッダ書き込み
    # uinit16_t：16ビット
    # PROGMEM：フラッシュメモリ保存
    f.write("const uint16_t hum[70*70] PROGMEM = {\n")
    for i, v in enumerate(data):
        f.write(f"0x{v:04X},")      # 4文字括り(0xFFFF)で描画
        if (i+1) % 12 == 0:         # 12画素ずつ書き込み
            f.write("\n")
    f.write("};")