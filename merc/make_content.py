"""Writes merc/data: the star map art, its operation files and the campaign file.
Run with Pillow when they change; the output is committed (argv: repo root)."""
import math
import pathlib
import random
import struct
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = pathlib.Path(sys.argv[1]) / "merc" / "data"
(ROOT / "art").mkdir(parents=True, exist_ok=True)
(ROOT / "campaign").mkdir(parents=True, exist_ok=True)

WORLDS = [
    dict(name="Carver V", x=48, y=196, color=(120, 200, 120), faction="the Chaos March",
         desc="A battered frontier world, and a good place to start.",
         champ="Sergeant Major Kell", shop="Purchase03", req=0, cost=0, tier=1,
         maps="mc2_02,mc2_03,mc2_05", enemies="48:Raven,46:Wolfhound,16:Hunchback,45:Werewolf",
         champs="16:Hunchback,874:Bushwacker,18:Catapult"),
    dict(name="Galatea", x=128, y=112, color=(120, 170, 255), faction="the Mercenary Guild",
         desc="The old hiring hall of the Inner Sphere, where better money means better enemies.",
         champ="Captain Oriel Voss", shop="Purchase12", req=1, cost=20000, tier=2,
         maps="mc2_09,mc2_11,mc2_12", enemies="874:Bushwacker,18:Catapult,47:Hollander,50:Jaegermech,15:Uller",
         champs="58:Zeus,18:Catapult,50:Jaegermech,52:Madcat"),
    dict(name="Outreach", x=208, y=40, color=(255, 190, 110), faction="the Dragoons",
         desc="Home of the best mercenaries alive, and they will test you.",
         champ="Colonel Hask", shop="Purchase22", req=2, cost=40000, tier=3,
         maps="mc2_17,mc2_19,mc2_22", enemies="58:Zeus,49:Highlander,52:Madcat,54:Vulture,51:Atlas",
         champs="51:Atlas,49:Highlander,52:Madcat,875:Bloodasp"),
]

# ---- art: 256x256 with the same stars every run
random.seed(3062)
img = Image.new("RGB", (256, 256), (4, 6, 16))
px = img.load()
for y in range(256):
    for x in range(256):
        d = abs((x - 128) + (y - 128)) / 128.0
        glow = max(0.0, 1.0 - d) ** 2 * (0.7 + 0.3 * math.sin(x * 0.07) * math.cos(y * 0.09))
        px[x, y] = (int(4 + 34 * glow), int(6 + 14 * glow), int(16 + 44 * glow))
draw = ImageDraw.Draw(img)
for _ in range(420):
    x, y, b = random.randrange(256), random.randrange(256), random.randrange(70, 235)
    draw.point((x, y), fill=(b, b, min(255, b + 20)))
for a, b in zip(WORLDS, WORLDS[1:]):
    draw.line((a["x"], a["y"], b["x"], b["y"]), fill=(70, 95, 130), width=1)
font = ImageFont.load_default()
for w in WORLDS:
    x, y = w["x"], w["y"]
    draw.ellipse((x - 6, y - 6, x + 6, y + 6), fill=w["color"], outline=(255, 255, 255))
    tw = draw.textlength(w["name"], font=font)
    tx = max(2, min(254 - tw, x - tw / 2))
    draw.text((tx, y + 16), w["name"], fill=(225, 230, 240), font=font)

# uncompressed top-left-origin 32-bit TGA, as the stock mcl_cm_op files are
pixels = bytearray()
for y in range(256):
    for x in range(256):
        r, g, b = img.getpixel((x, y))
        pixels += bytes((b, g, r, 255))
header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, 256, 256, 32, 0x28)
(ROOT / "art" / "mcl_merc_starmap.tga").write_bytes(header + bytes(pixels))


# ---- operation files: one view per world, its three contracts around it
def button(i, x, y, comment, rgb):
    c = "0xff%02X%02X%02X" % rgb
    dim = "0x47%02X%02X%02X" % rgb
    return f"""[Button{i}] // {comment}

l ID = XXX
l XLocation = {x}
l YLocation = {y}
l Width = 34
l Height = 39

l HelpCaption = 0
l HelpDesc = 0

st FileName = "mcl_cm_5.tga"

l UWidth = 51
l VHeight = 59

l UNormal = 51
l VNormal = 0

l UPressed = 0
l VPressed = 0

l UHighlight = 51
l VHighlight = 0

l UDisabled = 51
l VDisabled = 0

b AnimateBmp = 1
b AnimateText = 1

l NormalAnimationTimeStamps = 2
b NormalAnimationLoops = FALSE
f NormalTime0 = 0.0
l NormalColor0 = {c}
f NormalTime1 = 7.0
l NormalColor1 = {c}

l PressedAnimationTimeStamps = 2
b PressedAnimationLoops = FALSE
f PressedTime0 = 0.0
l PressedColor0 = {dim}
f PressedTime1 = 0.5
l PressedColor1 = {c}

l HighlightAnimationTimeStamps = 3
b HighlightAnimationLoops = TRUE
f HighlightTime0 = 0.0
l HighlightColor0 = {dim}
f HighlightTime1 = 0.3
l HighlightColor1 = {c}
f HighlightTime2 = 0.6
l HighlightColor2 = {dim}

l DisabledAnimationTimeStamps = 2
b DisabledAnimationLoops = FALSE
f DisabledTime0 = 0.0
l DisabledColor0 = 0xff546380
f DisabledTime1 = 0.5
l DisabledColor1 = 0xff546380

"""


CONTRACT = (255, 60, 40)
WORLD = (90, 200, 255)
for n, here in enumerate(WORLDS):
    # around the world, clear of the jump line, and inside the 256 square
    spots = [(here["x"] - 40, here["y"] - 44), (here["x"] + 8, here["y"] - 56), (here["x"] + 24, here["y"] - 14)]
    if here["x"] > 180:
        spots = [(here["x"] - 66, here["y"] - 26), (here["x"] - 56, here["y"] + 18), (here["x"] - 14, here["y"] + 26)]
    out = 'FITini\n\nb ChildCoordinatesAreRelative = 1\n\n'
    out += "[Buttons]\nl Buttoncount = 6\n\n"
    for i, (x, y) in enumerate(spots):
        out += button(i, max(0, min(222, x)), max(0, min(190, y)), f"contract {i}", CONTRACT)
    for i, w in enumerate(WORLDS):
        out += button(3 + i, w["x"] - 17, w["y"] - 20, "jump to " + w["name"], WORLD)
    out += '[Statics]\nl Staticcount = 1\n\n[Static0]\nl XLocation = 0\nl YLocation = 0\nl Width = 256\nl Height = 256\n'
    out += 'l UNormal = 0\nl VNormal = 0\nl UWidth = 256\nl VHeight = 256\nst FileName = "mcl_merc_starmap.tga"\n\n'
    out += "[Texts]\nl Textcount = 1\n\n[Text0]\nl XLocation = 4\nl YLocation = 230\nl Width = 248\nl Height = 22\n"
    out += "l Color = 0xffff0000\nb outline = FALSE\nl Font = 27656 // use agencyfb11\nl TextID = 27654\nl Alignment = 3\n\nFitEnd\n"
    (ROOT / "art" / f"mcl_merc_op{n}.fit").write_text(out, newline="\n")

# ---- the campaign
out = """FITini

// The mercenary mode's worlds: planning/future-features/OPEN-WORLD.md.
// Enemies and Champions are ObjectNumber:CSV name, as in a mission's [PartN].

[Campaign]

st CampaignName = "Mercenary Company (test)"
l GroupCount = 1
l CBills = 260000
st FinalVideo = ""

[Group0]

l NumberToComplete = 1
l MissionCount = 0
st OperationFile = "mcl_merc_op0"
st Video = "STANDIN"
l Tune = 22

[Merc]

"""
out += f"l NumWorlds = {len(WORLDS)}\nl StartWorld = 0\nl Seed = 3062\n\n"
for n, w in enumerate(WORLDS):
    out += f"""[MercWorld{n}]

st Name = "{w['name']}"
st Faction = "{w['faction']}"
st Description = "{w['desc']}"
st ChampionName = "{w['champ']}"
st Shop = "{w['shop']}"
st OperationFile = "mcl_merc_op{n}"
st Maps = "{w['maps']}"
st Enemies = "{w['enemies']}"
st Champions = "{w['champs']}"
l RequiredLicences = {w['req']}
l JumpCost = {w['cost']}
l Tier = {w['tier']}

"""
out += "FITend\n"
(ROOT / "campaign" / "merc.fit").write_text(out, newline="\n")
print("wrote", sorted(p.name for p in ROOT.rglob("*") if p.is_file()))
