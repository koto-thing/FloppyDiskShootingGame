import csv
from collections import defaultdict
from PIL import Image, ImageDraw

samples = defaultdict(list)
with open('Temp/HomingLaunchCapture.csv', encoding='utf-8-sig') as stream:
    for row in csv.DictReader(stream):
        samples[int(row['slot'])].append((int(row['frame']), float(row['x']), float(row['y']), int(row['active'])))

def screen(x, y):
    return (int(240 + y * 420), int(530 - x * 330))

frames = []
for frame in range(120):
    canvas = Image.new('RGB', (480, 640), '#07101e')
    draw = ImageDraw.Draw(canvas)
    draw.text((18, 16), 'ACTUAL SHOT POSITIONS / SIDE VIEW ROTATED', fill='#a9c0d7')
    draw.text((18, 36), f'{frame / 60:.2f} s   |   3 volleys', fill='#a9c0d7')
    for yy in range(100, 620, 80):
        draw.line((24, yy, 456, yy), fill='#142332')
    ex, ey = screen(1.2, 0)
    draw.ellipse((ex - 9, ey - 9, ex + 9, ey + 9), fill='#ff984a')
    draw.text((ex + 16, ey - 6), 'TARGET', fill='#ffba76')
    px, py = screen(0, 0)
    draw.polygon(((px, py - 10), (px - 9, py + 8), (px + 9, py + 8)), fill='#c2d5e5')
    for slot, records in samples.items():
        visible = [row for row in records if frame - 24 <= row[0] <= frame and row[3]]
        for a, b in zip(visible, visible[1:]):
            fade = max(0, 1 - (frame - b[0]) / 24)
            color = (int(70 * fade), int(165 * fade), int(255 * fade))
            draw.line((*screen(a[1], a[2]), *screen(b[1], b[2])), fill=color, width=max(1, int(4 * fade)))
        heads = [row for row in records if row[0] == frame and row[3]]
        if heads:
            hx, hy = screen(heads[0][1], heads[0][2])
            draw.ellipse((hx - 3, hy - 3, hx + 3, hy + 3), fill='#f2ffff')
    draw.text((18, 614), 'Path preview from game code (not a game recording)', fill='#71869a')
    frames.append(canvas)
frames[0].save('Temp/HomingLaunchTrajectory.gif', save_all=True, append_images=frames[1:], duration=17, loop=0)
frames[35].save('Temp/HomingLaunchTrajectory.png')
