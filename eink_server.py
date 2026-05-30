from mcp.server.fastmcp import FastMCP
from PIL import Image, ImageDraw, ImageFont
import socket
import struct
import os
import io
import math

mcp = FastMCP("eink-display")

W, H = 800, 480
ESP32_IP = os.environ.get("ESP32_IP") or "192.168.1.240"
ESP32_PORT = int(os.environ.get("ESP32_PORT") or "8080")

# ---------------------------------------------------------------------------
# Font discovery
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
FONTS_DIR = os.path.join(SCRIPT_DIR, "fonts")


def _find_font_file(name: str) -> str | None:
    candidates = [
        os.path.join(FONTS_DIR, name),
        os.path.join(SCRIPT_DIR, "node_modules", "@fortawesome",
                     "fontawesome-free", "webfonts", name),
    ]
    for p in candidates:
        if os.path.exists(p):
            return p
    return None


FA_SOLID_PATH = _find_font_file("fa-solid-900.ttf")
FA_REGULAR_PATH = _find_font_file("fa-regular-400.ttf")
WI_PATH = _find_font_file("weathericons-regular-webfont.ttf")

# ---------------------------------------------------------------------------
# FontAwesome 6 Free — Solid glyph map  (~270 icons)
# ---------------------------------------------------------------------------
FA_ICONS: dict[str, str] = {
    # ── Weather & sky ──────────────────────────────────────────────────────
    "sun":                   "",
    "moon":                  "",
    "cloud":                 "",
    "cloud-sun":             "",
    "cloud-moon":            "",
    "cloud-rain":            "",
    "cloud-showers-heavy":   "",
    "cloud-showers-water":   "",
    "cloud-bolt":            "",
    "cloud-sun-rain":        "",
    "cloud-moon-rain":       "",
    "snowflake":             "",
    "wind":                  "",
    "tornado":               "",
    "hurricane":             "",
    "smog":                  "",
    "meteor":                "",
    "rainbow":               "",
    "umbrella":              "",
    "droplet":               "",
    "temperature-full":      "",
    "temperature-high":      "",
    "temperature-half":      "",
    "temperature-low":       "",
    "temperature-empty":     "",
    "temperature-arrow-up":  "",
    "temperature-arrow-down":"",
    "water":                 "",

    # ── Nature & animals ───────────────────────────────────────────────────
    "fire":           "",
    "fire-flame-curved": "",
    "leaf":           "",
    "tree":           "",
    "seedling":       "",
    "mountain":       "",
    "mountain-sun":   "",
    "fish":           "",
    "fish-fins":      "",
    "shrimp":         "",
    "dove":           "",
    "feather":        "",
    "feather-pointed":"",
    "paw":            "",
    "bug":            "",
    "spider":         "",
    "worm":           "",
    "frog":           "",
    "crow":           "",
    "dragon":         "",
    "hippo":          "",
    "horse":          "",
    "otter":          "",
    "dog":            "",
    "cat":            "",
    "kiwi-bird":      "",
    "locust":         "",

    # ── Arrows & navigation ───────────────────────────────────────────────
    "arrow-up":            "",
    "arrow-down":          "",
    "arrow-left":          "",
    "arrow-right":         "",
    "arrows-rotate":       "",
    "arrow-rotate-left":   "",
    "arrow-rotate-right":  "",
    "arrows-up-down":      "",
    "arrows-left-right":   "",
    "up-down-left-right":  "",
    "chevron-up":          "",
    "chevron-down":        "",
    "chevron-left":        "",
    "chevron-right":       "",
    "angles-up":           "",
    "angles-down":         "",
    "angles-left":         "",
    "angles-right":        "",
    "circle-arrow-up":     "",
    "circle-arrow-down":   "",
    "circle-arrow-left":   "",
    "circle-arrow-right":  "",
    "right-from-bracket":  "",
    "right-to-bracket":    "",
    "location-dot":        "",
    "location-crosshairs": "",
    "compass":             "",
    "route":               "",
    "map":                 "",
    "map-pin":             "",
    "signs-post":          "",

    # ── Status & feedback ─────────────────────────────────────────────────
    "check":                "",
    "xmark":                "",
    "circle-check":         "",
    "circle-xmark":         "",
    "circle-info":          "",
    "circle-exclamation":   "",
    "circle-question":      "",
    "circle-plus":          "",
    "circle-minus":         "",
    "triangle-exclamation": "",
    "radiation":            "",
    "biohazard":            "",
    "skull-crossbones":     "",
    "thumbs-up":            "",
    "thumbs-down":          "",
    "flag":                 "",
    "flag-checkered":       "",
    "bookmark":             "",
    "ban":                  "",
    "shield":               "",
    "shield-halved":        "",
    "lock":                 "",
    "lock-open":            "",
    "unlock":               "",
    "key":                  "",
    "fingerprint":          "",

    # ── UI & interface ────────────────────────────────────────────────────
    "bars":                "",
    "ellipsis":            "",
    "ellipsis-vertical":   "",
    "gear":                "",
    "gears":               "",
    "sliders":             "",
    "magnifying-glass":    "",
    "magnifying-glass-plus":  "",
    "magnifying-glass-minus": "",
    "eye":                 "",
    "eye-slash":           "",
    "pen":                 "",
    "pencil":              "",
    "pen-to-square":       "",
    "eraser":              "",
    "trash":               "",
    "trash-can":           "",
    "plus":                "",
    "minus":               "",
    "toggle-on":           "",
    "toggle-off":          "",
    "filter":              "",
    "sort":                "",
    "sort-up":             "",
    "sort-down":           "",
    "grip":                "",
    "grip-vertical":       "",
    "expand":              "",
    "compress":            "",
    "maximize":            "",
    "minimize":            "",
    "crop":                "",
    "scissors":            "",
    "copy":                "",
    "paste":               "",
    "clipboard":           "",
    "clipboard-check":     "",

    # ── Shapes & symbols ──────────────────────────────────────────────────
    "heart":         "",
    "star":          "",
    "star-half":     "",
    "circle":        "",
    "square":        "",
    "diamond":       "",
    "play":          "",
    "infinity":      "",
    "hashtag":       "",
    "at":            "",
    "percent":       "",
    "bolt":          "",
    "crown":         "",
    "cross":         "",
    "certificate":   "",

    # ── Objects & things ──────────────────────────────────────────────────
    "house":          "",
    "building":       "",
    "store":          "",
    "shop":           "",
    "bell":           "",
    "bell-slash":     "",
    "lightbulb":      "",
    "plug":           "",
    "battery-full":   "",
    "battery-three-quarters": "",
    "battery-half":   "",
    "battery-quarter":"",
    "battery-empty":  "",
    "wrench":         "",
    "hammer":         "",
    "screwdriver-wrench": "",
    "toolbox":        "",
    "briefcase":      "",
    "suitcase":       "",
    "box":            "",
    "gift":           "",
    "trophy":         "",
    "medal":          "",
    "award":          "",
    "graduation-cap": "",
    "book":           "",
    "book-open":      "",
    "newspaper":      "",
    "calendar":       "",
    "calendar-days":  "",
    "clock":          "",
    "hourglass":      "",
    "hourglass-half": "",
    "stopwatch":      "",
    "alarm-clock":    "",
    "utensils":       "",
    "mug-hot":        "",
    "mug-saucer":     "",
    "wine-glass":     "",
    "martini-glass":  "",
    "beer-mug-empty": "",
    "pizza-slice":    "",
    "cookie":         "",
    "apple-whole":    "",
    "carrot":         "",
    "lemon":          "",
    "candy-cane":     "",
    "ice-cream":      "",
    "cake-candles":   "",

    # ── People & users ────────────────────────────────────────────────────
    "user":           "",
    "users":          "",
    "user-plus":      "",
    "user-minus":     "",
    "user-gear":      "",
    "user-shield":    "",
    "user-doctor":    "",
    "person":         "",
    "person-walking": "",
    "person-running": "",
    "person-biking":  "",
    "person-swimming":"",
    "children":       "",
    "baby":           "",
    "hand":           "",
    "hands-clapping": "",
    "handshake":      "",
    "face-smile":     "",
    "face-frown":     "",
    "face-meh":       "",
    "face-laugh":     "",
    "face-surprise":  "",
    "face-angry":     "",
    "skull":          "",
    "ghost":          "",

    # ── Technology & devices ──────────────────────────────────────────────
    "wifi":            "",
    "signal":          "",
    "satellite-dish":  "",
    "tower-broadcast": "",
    "laptop":          "",
    "desktop":         "",
    "mobile":          "",
    "tablet":          "",
    "keyboard":        "",
    "print":           "",
    "camera":          "",
    "video":           "",
    "microphone":      "",
    "microphone-slash":"",
    "headphones":      "",
    "tv":              "",
    "server":          "",
    "hard-drive":      "",
    "microchip":       "",
    "robot":           "",
    "code":            "",
    "terminal":        "",
    "database":        "",
    "qrcode":          "",
    "barcode":         "",
    "power-off":       "",
    "plug-circle-bolt":"",

    # ── Communication ─────────────────────────────────────────────────────
    "phone":          "",
    "phone-volume":   "",
    "envelope":       "",
    "envelope-open":  "",
    "inbox":          "",
    "comment":        "",
    "comments":       "",
    "message":        "",
    "paper-plane":    "",
    "share":          "",
    "share-nodes":    "",
    "rss":            "",
    "bullhorn":       "",

    # ── Charts & data ─────────────────────────────────────────────────────
    "chart-bar":      "",
    "chart-line":     "",
    "chart-pie":      "",
    "chart-area":     "",
    "chart-column":   "",
    "table":          "",
    "table-cells":    "",
    "list":           "",
    "list-check":     "",
    "list-ol":        "",
    "list-ul":        "",
    "gauge":          "",
    "gauge-high":     "",
    "gauge-simple":   "",

    # ── Files & documents ─────────────────────────────────────────────────
    "file":           "",
    "file-lines":     "",
    "file-code":      "",
    "file-image":     "",
    "file-pdf":       "",
    "file-excel":     "",
    "file-zipper":    "",
    "file-arrow-up":  "",
    "file-arrow-down":"",
    "folder":         "",
    "folder-open":    "",
    "download":       "",
    "upload":         "",
    "cloud-arrow-up": "",
    "cloud-arrow-down":"",
    "floppy-disk":    "",

    # ── Media & playback ──────────────────────────────────────────────────
    "pause":           "",
    "stop":            "",
    "forward":         "",
    "backward":        "",
    "forward-step":    "",
    "backward-step":   "",
    "forward-fast":    "",
    "backward-fast":   "",
    "shuffle":         "",
    "repeat":          "",
    "volume-high":     "",
    "volume-low":      "",
    "volume-off":      "",
    "volume-xmark":    "",
    "music":           "",
    "image":           "",
    "film":            "",
    "palette":         "",
    "paintbrush":      "",
    "spray-can":       "",

    # ── Transport ─────────────────────────────────────────────────────────
    "car":          "",
    "car-side":     "",
    "truck":        "",
    "bus":          "",
    "taxi":         "",
    "motorcycle":   "",
    "bicycle":      "",
    "plane":        "",
    "plane-departure":"",
    "plane-arrival": "",
    "helicopter":   "",
    "ship":         "",
    "sailboat":     "",
    "train":        "",
    "train-subway": "",
    "rocket":       "",
    "shuttle-space":"",
    "gas-pump":     "",
    "road":         "",
    "trailer":      "",

    # ── Health & science ──────────────────────────────────────────────────
    "heart-pulse":    "",
    "stethoscope":    "",
    "syringe":        "",
    "pills":          "",
    "capsules":       "",
    "vial":           "",
    "vials":          "",
    "microscope":     "",
    "flask":          "",
    "flask-vial":     "",
    "atom":           "",
    "dna":            "",
    "virus":          "",
    "lungs":          "",
    "brain":          "",
    "bone":           "",
    "tooth":          "",
    "hospital":       "",
    "bandage":        "",
    "wheelchair":     "",

    # ── Commerce & money ──────────────────────────────────────────────────
    "cart-shopping":  "",
    "bag-shopping":   "",
    "basket-shopping":"",
    "credit-card":    "",
    "money-bill":     "",
    "coins":          "",
    "wallet":         "",
    "receipt":        "",
    "cash-register":  "",
    "tags":           "",
    "tag":            "",
    "barcode":        "",
    "percent":        "",

    # ── Globe & accessibility ─────────────────────────────────────────────
    "globe":           "",
    "earth-americas":  "",
    "earth-europe":    "",
    "earth-asia":      "",
    "earth-africa":    "",
    "language":        "",
    "universal-access":"",

    # ── Misc ──────────────────────────────────────────────────────────────
    "anchor":         "",
    "dice":           "",
    "puzzle-piece":   "",
    "gamepad":        "",
    "chess":          "",
    "palette":        "",
    "masks-theater":  "",
    "wand-magic-sparkles": "",
    "hat-wizard":     "",
    "broom":          "",
    "campground":     "",
    "tent":           "",
    "mountain-city":  "",
    "city":           "",
    "landmark":       "",
    "monument":       "",
    "dungeon":        "",
    "place-of-worship":"",
    "recycle":        "",
    "leaf":           "",
    "solar-panel":    "",
    "wind-turbine":   "",  # not in free; alias below
}

# ---------------------------------------------------------------------------
# Weather Icons font glyph map  (~80 icons)
# ---------------------------------------------------------------------------
WI_ICONS: dict[str, str] = {
    # ── Day conditions ────────────────────────────────────────────────────
    "wi-day-sunny":             "",
    "wi-day-cloudy":            "",
    "wi-day-cloudy-gusts":      "",
    "wi-day-cloudy-windy":      "",  # note: some refs say f011
    "wi-day-fog":               "",
    "wi-day-hail":              "",
    "wi-day-haze":              "",
    "wi-day-lightning":         "",
    "wi-day-rain":              "",
    "wi-day-rain-mix":          "",
    "wi-day-rain-wind":         "",
    "wi-day-showers":           "",
    "wi-day-sleet":             "",
    "wi-day-sleet-storm":       "",
    "wi-day-snow":              "",
    "wi-day-snow-thunderstorm": "",
    "wi-day-snow-wind":         "",
    "wi-day-sprinkle":          "",
    "wi-day-storm-showers":     "",
    "wi-day-sunny-overcast":    "",
    "wi-day-thunderstorm":      "",
    "wi-day-windy":             "",
    "wi-day-hot":               "",

    # ── Night conditions ──────────────────────────────────────────────────
    "wi-night-clear":              "",
    "wi-night-alt-cloudy":         "",
    "wi-night-alt-cloudy-gusts":   "",
    "wi-night-alt-cloudy-windy":   "",
    "wi-night-alt-hail":           "",
    "wi-night-alt-lightning":      "",
    "wi-night-alt-rain":           "",
    "wi-night-alt-rain-mix":       "",
    "wi-night-alt-rain-wind":      "",
    "wi-night-alt-showers":        "",
    "wi-night-alt-sleet":          "",
    "wi-night-alt-snow":           "",
    "wi-night-alt-snow-thunderstorm": "",
    "wi-night-alt-snow-wind":      "",
    "wi-night-alt-sprinkle":       "",
    "wi-night-alt-storm-showers":  "",
    "wi-night-alt-thunderstorm":   "",
    "wi-night-fog":                "",
    "wi-night-cloudy":             "",
    "wi-night-cloudy-gusts":       "",
    "wi-night-cloudy-windy":       "",
    "wi-night-hail":               "",
    "wi-night-lightning":          "",
    "wi-night-rain":               "",
    "wi-night-rain-mix":           "",
    "wi-night-rain-wind":          "",
    "wi-night-showers":            "",
    "wi-night-sleet":              "",
    "wi-night-snow":               "",
    "wi-night-snow-thunderstorm":  "",
    "wi-night-snow-wind":          "",
    "wi-night-sprinkle":           "",
    "wi-night-storm-showers":      "",
    "wi-night-thunderstorm":       "",

    # ── Neutral / general ─────────────────────────────────────────────────
    "wi-cloud":              "",
    "wi-cloudy":             "",
    "wi-cloudy-gusts":       "",
    "wi-cloudy-windy":       "",
    "wi-fog":                "",
    "wi-hail":               "",
    "wi-rain":               "",
    "wi-rain-mix":           "",
    "wi-rain-wind":          "",
    "wi-showers":            "",
    "wi-sleet":              "",
    "wi-snow":               "",
    "wi-snow-wind":          "",
    "wi-sprinkle":           "",
    "wi-storm-showers":      "",
    "wi-thunderstorm":       "",
    "wi-strong-wind":        "",
    "wi-hot":                "",
    "wi-tornado":            "",
    "wi-hurricane":          "",
    "wi-dust":               "",
    "wi-sandstorm":          "",
    "wi-earthquake":         "",
    "wi-fire":               "",
    "wi-flood":              "",
    "wi-volcano":            "",
    "wi-tsunami":            "",
    "wi-lightning":          "",
    "wi-smoke":              "",
    "wi-smog":               "",
    "wi-windy":              "",
    "wi-snowflake-cold":     "",

    # ── Instruments & misc ────────────────────────────────────────────────
    "wi-thermometer":          "",
    "wi-thermometer-exterior": "",
    "wi-thermometer-internal": "",
    "wi-barometer":            "",
    "wi-humidity":             "",
    "wi-celsius":              "",
    "wi-fahrenheit":           "",
    "wi-degrees":              "",
    "wi-raindrop":             "",
    "wi-raindrops":            "",
    "wi-umbrella":             "",
    "wi-sunrise":              "",
    "wi-sunset":               "",
    "wi-wind-direction":       "",
    "wi-horizon":              "",
    "wi-horizon-alt":          "",
    "wi-stars":                "",
    "wi-na":                   "",

    # ── Moon phases ───────────────────────────────────────────────────────
    "wi-moon-new":                "",
    "wi-moon-waxing-crescent-1":  "",
    "wi-moon-waxing-crescent-2":  "",
    "wi-moon-waxing-crescent-3":  "",
    "wi-moon-waxing-crescent-4":  "",
    "wi-moon-waxing-crescent-5":  "",
    "wi-moon-waxing-crescent-6":  "",
    "wi-moon-first-quarter":      "",
    "wi-moon-waxing-gibbous-1":   "",
    "wi-moon-waxing-gibbous-2":   "",
    "wi-moon-waxing-gibbous-3":   "",
    "wi-moon-waxing-gibbous-4":   "",
    "wi-moon-waxing-gibbous-5":   "",
    "wi-moon-waxing-gibbous-6":   "",
    "wi-moon-full":               "",
    "wi-moon-waning-gibbous-1":   "",
    "wi-moon-waning-gibbous-2":   "",
    "wi-moon-waning-gibbous-3":   "",
    "wi-moon-waning-gibbous-4":   "",
    "wi-moon-waning-gibbous-5":   "",
    "wi-moon-waning-gibbous-6":   "",
    "wi-moon-third-quarter":      "",
    "wi-moon-waning-crescent-1":  "",
    "wi-moon-waning-crescent-2":  "",
    "wi-moon-waning-crescent-3":  "",
    "wi-moon-waning-crescent-4":  "",
    "wi-moon-waning-crescent-5":  "",
    "wi-moon-waning-crescent-6":  "",
}

# ---------------------------------------------------------------------------
# Convenience aliases — short or colloquial names that map to canonical keys
# ---------------------------------------------------------------------------
ICON_ALIASES: dict[str, str] = {
    # Weather shorthand → FontAwesome
    "sunny":        "sun",
    "cloudy":       "cloud",
    "rainy":        "cloud-rain",
    "snowy":        "snowflake",
    "stormy":       "cloud-bolt",
    "thunderstorm": "cloud-bolt",
    "foggy":        "smog",
    "windy":        "wind",
    "hot":          "temperature-high",
    "cold":         "temperature-low",
    "rain":         "cloud-rain",
    "snow":         "snowflake",
    "fog":          "smog",
    "haze":         "smog",
    "drizzle":      "cloud-rain",
    "blizzard":     "snowflake",
    "thermometer":  "temperature-half",

    # Common alternate names
    "home":         "house",
    "cog":          "gear",
    "cogs":         "gears",
    "search":       "magnifying-glass",
    "close":        "xmark",
    "x":            "xmark",
    "times":        "xmark",
    "warning":      "triangle-exclamation",
    "alert":        "triangle-exclamation",
    "danger":       "triangle-exclamation",
    "info":         "circle-info",
    "question":     "circle-question",
    "error":        "circle-xmark",
    "success":      "circle-check",
    "ok":           "circle-check",
    "refresh":      "arrows-rotate",
    "reload":       "arrows-rotate",
    "undo":         "arrow-rotate-left",
    "redo":         "arrow-rotate-right",
    "save":         "floppy-disk",
    "delete":       "trash",
    "remove":       "trash-can",
    "edit":         "pen-to-square",
    "settings":     "gear",
    "config":       "sliders",
    "mail":         "envelope",
    "email":        "envelope",
    "phone-call":   "phone-volume",
    "chat":         "comment",
    "send":         "paper-plane",
    "login":        "right-to-bracket",
    "logout":       "right-from-bracket",
    "menu":         "bars",
    "hamburger":    "bars",
    "more":         "ellipsis",
    "options":      "ellipsis-vertical",
    "profile":      "user",
    "account":      "user",
    "group":        "users",
    "team":         "users",
    "like":         "thumbs-up",
    "dislike":      "thumbs-down",
    "favorite":     "heart",
    "love":         "heart",
    "pin":          "map-pin",
    "location":     "location-dot",
    "gps":          "location-crosshairs",
    "navigate":     "compass",
    "money":        "money-bill",
    "payment":      "credit-card",
    "cart":         "cart-shopping",
    "shop":         "store",
    "wifi-signal":  "wifi",
    "bluetooth":    "signal",
    "power":        "power-off",
    "on-off":       "power-off",
    "computer":     "desktop",
    "monitor":      "desktop",
    "smartphone":   "mobile",
    "mic":          "microphone",
    "mic-off":      "microphone-slash",
    "speaker":      "volume-high",
    "mute":         "volume-xmark",
    "movie":        "film",
    "photo":        "image",
    "picture":      "image",
    "graph":        "chart-line",
    "bar-chart":    "chart-bar",
    "pie-chart":    "chart-pie",
    "area-chart":   "chart-area",
    "dashboard":    "gauge",
    "meter":        "gauge-high",
    "speedometer":  "gauge-high",
    "checklist":    "list-check",
    "todo":         "list-check",
    "document":     "file-lines",
    "doc":          "file-lines",
    "pdf":          "file-pdf",
    "zip":          "file-zipper",
    "spreadsheet":  "file-excel",
    "attach":       "paperclip",
    "link":         "link",
    "magic":        "wand-magic-sparkles",
    "wizard":       "hat-wizard",
    "science":      "flask",
    "lab":          "flask-vial",
    "health":       "heart-pulse",
    "medical":      "stethoscope",
    "emergency":    "hospital",
    "food":         "utensils",
    "coffee":       "mug-hot",
    "tea":          "mug-saucer",
    "beer":         "beer-mug-empty",
    "wine":         "wine-glass",
    "cocktail":     "martini-glass",
    "birthday":     "cake-candles",
    "celebration":  "cake-candles",
    "happy":        "face-smile",
    "sad":          "face-frown",
    "neutral":      "face-meh",
    "laugh":        "face-laugh",
    "surprised":    "face-surprise",
    "angry":        "face-angry",
    "recycle":      "recycle",
    "eco":          "leaf",
    "green":        "leaf",
    "solar":        "solar-panel",
    "airplane":     "plane",
    "flight":       "plane-departure",
    "landing":      "plane-arrival",
    "drive":        "car",
    "vehicle":      "car-side",
    "ride":         "bicycle",
    "swim":         "person-swimming",
    "run":          "person-running",
    "walk":         "person-walking",
    "cycle":        "person-biking",
    "pet":          "paw",
    "animal":       "paw",
    "secure":       "shield-halved",
    "protected":    "shield",
    "fingerprint":  "fingerprint",
    "auth":         "fingerprint",
    "lightbulb":    "lightbulb",
    "idea":         "lightbulb",
    "battery":      "battery-full",
    "charge":       "battery-full",
    "tools":        "screwdriver-wrench",
    "repair":       "wrench",
    "build":        "hammer",
    "package":      "box",
    "present":      "gift",
    "achievement":  "trophy",
    "learn":        "graduation-cap",
    "study":        "book",
    "read":         "book-open",
    "news":         "newspaper",
    "schedule":     "calendar-days",
    "timer":        "stopwatch",
    "hourglass":    "hourglass-half",
    "time":         "clock",
    "cut":          "scissors",
    "clip":         "clipboard",
    "checked":      "clipboard-check",
    "game":         "gamepad",
    "puzzle":       "puzzle-piece",
    "theater":      "masks-theater",
    "camping":      "campground",
    "city":         "city",
    "world":        "globe",
    "earth":        "earth-americas",
    "translate":    "language",
    "accessibility":"universal-access",
}

# Collect all known icon names for the tool description
_ALL_ICON_NAMES = sorted(
    set(list(FA_ICONS.keys()) + list(WI_ICONS.keys()) + list(ICON_ALIASES.keys()))
)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def get_font(size: int = 24, bold: bool = False):
    candidates = []
    if bold:
        candidates += [
            "DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
            "C:\\Windows\\Fonts\\arialbd.ttf",
        ]
    candidates += [
        "DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    ]
    for path in candidates:
        try:
            return ImageFont.truetype(path, size)
        except (IOError, OSError):
            continue
    try:
        return ImageFont.load_default(size)
    except TypeError:
        return ImageFont.load_default()


def to_bmp(img: Image.Image) -> bytes:
    if img.mode != "1":
        img = img.convert("1")
    buf = io.BytesIO()
    img.save(buf, format="BMP")
    return buf.getvalue()


def send_to_esp32(data: bytes) -> str:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10)
    try:
        sock.connect((ESP32_IP, ESP32_PORT))
        sock.sendall(struct.pack("<I", len(data)))
        sock.sendall(data)
        response = sock.recv(1024)
        return response.decode()
    finally:
        sock.close()


# ---------------------------------------------------------------------------
# Icon renderer
# ---------------------------------------------------------------------------

def draw_icon(draw: ImageDraw.ImageDraw, name: str, x: int, y: int, size: int):
    resolved = ICON_ALIASES.get(name, name)

    # FontAwesome Solid
    if resolved in FA_ICONS and FA_SOLID_PATH:
        try:
            font = ImageFont.truetype(FA_SOLID_PATH, size)
            draw.text((x, y), FA_ICONS[resolved], fill=0, font=font)
            return
        except Exception:
            pass

    # Weather Icons
    if resolved in WI_ICONS and WI_PATH:
        try:
            font = ImageFont.truetype(WI_PATH, size)
            draw.text((x, y), WI_ICONS[resolved], fill=0, font=font)
            return
        except Exception:
            pass

    # Try original name directly in WI (agent may pass wi-* names)
    if name in WI_ICONS and WI_PATH:
        try:
            font = ImageFont.truetype(WI_PATH, size)
            draw.text((x, y), WI_ICONS[name], fill=0, font=font)
            return
        except Exception:
            pass

    # Fallback: render the name as text in a box
    draw.rectangle([x, y, x + size, y + size], outline=0, width=2)
    f = get_font(max(10, size // 4))
    label = resolved[:6]
    draw.text((x + 4, y + size // 3), label, fill=0, font=f)


# ---------------------------------------------------------------------------
# MCP tools
# ---------------------------------------------------------------------------

@mcp.tool()
def send_to_display(lines: list[str]) -> str:
    """Render text lines onto the 800x480 e-ink display connected via ESP32.
    Each string in `lines` is drawn on its own row."""
    try:
        img = Image.new("1", (W, H), 1)
        draw = ImageDraw.Draw(img)
        font = get_font(28)
        y = 40
        for line in lines:
            draw.text((40, y), line, fill=0, font=font)
            y += 50
        data = to_bmp(img)
        response = send_to_esp32(data)
        return f"Sent {len(data)} bytes to {ESP32_IP}:{ESP32_PORT}. ESP32: {response}"
    except ConnectionRefusedError:
        return f"ERROR: Connection refused at {ESP32_IP}:{ESP32_PORT}. Is the ESP32 powered on?"
    except socket.timeout:
        return f"ERROR: Timeout reaching {ESP32_IP}:{ESP32_PORT}."
    except Exception as e:
        return f"ERROR: {type(e).__name__}: {e}"


@mcp.tool()
def draw_on_display(elements: list[dict]) -> str:
    """Draw shapes, text, icons, and graphics on the 800x480 e-ink display.

    Each element is a dict with a "type" key and type-specific properties.
    Canvas is 800x480, origin at top-left. Black on white (1-bit).

    Element types:
      rect      — x, y, width, height, fill (bool), stroke (int, default 2)
      circle    — x, y (center), radius, fill (bool), stroke (int)
      ellipse   — x, y, width, height, fill (bool), stroke (int)
      line      — x1, y1, x2, y2, width (default 2)
      polygon   — points ([[x,y],...]), fill (bool)
      arc       — x, y, width, height, start (deg), end (deg), stroke (int)
      text      — x, y, content, size (default 24), bold (bool)
      icon      — name, x, y, size (default 48), style ("solid" or "regular")

    Icons use FontAwesome 6 and Weather Icons fonts. 270+ FA icons, 120+ WI
    icons, and 170+ aliases. Examples:

      FontAwesome: sun, moon, cloud, heart, star, check, xmark, house, gear,
        bell, user, wifi, bolt, fire, leaf, car, plane, rocket, chart-line,
        chart-bar, chart-pie, gauge, thermometer, snowflake, wind, tornado,
        umbrella, cloud-rain, cloud-bolt, cloud-sun, temperature-high,
        temperature-low, droplet, water, mountain, fish, tree, dog, cat,
        robot, microchip, code, database, server, camera, lock, shield,
        flag, trophy, brain, dna, flask, atom, stethoscope, pills, hospital

      Weather Icons (wi-* prefix): wi-day-sunny, wi-night-clear,
        wi-day-cloudy, wi-day-rain, wi-day-snow, wi-day-thunderstorm,
        wi-night-alt-rain, wi-cloudy, wi-rain, wi-snow, wi-thunderstorm,
        wi-fog, wi-hot, wi-tornado, wi-hurricane, wi-strong-wind,
        wi-thermometer, wi-barometer, wi-humidity, wi-sunrise, wi-sunset,
        wi-moon-full, wi-moon-new, wi-moon-first-quarter, wi-raindrops

      Aliases: sunny, cloudy, rainy, snowy, stormy, foggy, windy, hot, cold,
        home, search, warning, refresh, save, delete, edit, settings, mail,
        chat, send, menu, like, dislike, favorite, location, money, cart,
        power, graph, dashboard, coffee, happy, sad, angry, world, tools
    """
    try:
        img = Image.new("1", (W, H), 1)
        draw = ImageDraw.Draw(img)

        for el in elements:
            t = el.get("type", "")

            if t == "rect":
                x0, y0 = el.get("x", 0), el.get("y", 0)
                x1 = x0 + el.get("width", 100)
                y1 = y0 + el.get("height", 50)
                if el.get("fill"):
                    draw.rectangle([x0, y0, x1, y1], fill=0)
                else:
                    draw.rectangle([x0, y0, x1, y1], outline=0, width=el.get("stroke", 2))

            elif t == "circle":
                cx, cy, r = el.get("x", 0), el.get("y", 0), el.get("radius", 50)
                if el.get("fill"):
                    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=0)
                else:
                    draw.ellipse([cx - r, cy - r, cx + r, cy + r], outline=0, width=el.get("stroke", 2))

            elif t == "ellipse":
                x0, y0 = el.get("x", 0), el.get("y", 0)
                x1 = x0 + el.get("width", 100)
                y1 = y0 + el.get("height", 50)
                if el.get("fill"):
                    draw.ellipse([x0, y0, x1, y1], fill=0)
                else:
                    draw.ellipse([x0, y0, x1, y1], outline=0, width=el.get("stroke", 2))

            elif t == "line":
                draw.line(
                    [(el.get("x1", 0), el.get("y1", 0)),
                     (el.get("x2", 100), el.get("y2", 100))],
                    fill=0, width=el.get("width", 2),
                )

            elif t == "polygon":
                pts = [tuple(p) for p in el.get("points", [])]
                if len(pts) >= 3:
                    if el.get("fill"):
                        draw.polygon(pts, fill=0)
                    else:
                        draw.polygon(pts, outline=0)

            elif t == "arc":
                x0, y0 = el.get("x", 0), el.get("y", 0)
                x1 = x0 + el.get("width", 100)
                y1 = y0 + el.get("height", 100)
                draw.arc([x0, y0, x1, y1], el.get("start", 0), el.get("end", 180),
                         fill=0, width=el.get("stroke", 2))

            elif t == "text":
                font = get_font(el.get("size", 24), el.get("bold", False))
                draw.text(
                    (el.get("x", 0), el.get("y", 0)),
                    el.get("content", ""), fill=0, font=font,
                )

            elif t == "icon":
                draw_icon(draw, el.get("name", ""),
                          el.get("x", 0), el.get("y", 0), el.get("size", 48))

        data = to_bmp(img)
        response = send_to_esp32(data)
        return f"Drew {len(elements)} elements. Sent {len(data)} bytes. ESP32: {response}"
    except ConnectionRefusedError:
        return f"ERROR: Connection refused at {ESP32_IP}:{ESP32_PORT}. Is the ESP32 powered on?"
    except socket.timeout:
        return f"ERROR: Timeout reaching {ESP32_IP}:{ESP32_PORT}."
    except Exception as e:
        return f"ERROR: {type(e).__name__}: {e}"


@mcp.tool()
def send_svg_to_display(svg: str) -> str:
    """Render an SVG string onto the 800x480 e-ink display.

    The SVG is rasterized to 800x480 monochrome. Use viewBox="0 0 800 480"
    for pixel-accurate layout. Dark colours become black, light become white.

    Requires cairosvg (pip install cairosvg).
    """
    try:
        import cairosvg
    except ImportError:
        return "ERROR: cairosvg is not installed. Run: pip install cairosvg"

    try:
        png_data = cairosvg.svg2png(bytestring=svg.encode("utf-8"),
                                    output_width=W, output_height=H)
        img = Image.open(io.BytesIO(png_data)).convert("L")
        img = img.point(lambda p: 255 if p > 128 else 0, mode="1")
        data = to_bmp(img)
        response = send_to_esp32(data)
        return f"Rendered SVG ({len(svg)} chars). Sent {len(data)} bytes. ESP32: {response}"
    except ConnectionRefusedError:
        return f"ERROR: Connection refused at {ESP32_IP}:{ESP32_PORT}. Is the ESP32 powered on?"
    except socket.timeout:
        return f"ERROR: Timeout reaching {ESP32_IP}:{ESP32_PORT}."
    except Exception as e:
        return f"ERROR: {type(e).__name__}: {e}"


@mcp.tool()
def list_icons(category: str = "") -> str:
    """List available icon names, optionally filtered by category.

    Categories: weather, wi (weather-icons font), nature, arrows, status,
    ui, shapes, objects, people, tech, communication, charts, files, media,
    transport, health, commerce, aliases, all
    """
    groups: dict[str, list[str]] = {
        "weather": [k for k in FA_ICONS if k.startswith(("sun", "moon", "cloud", "snow",
                    "wind", "tornado", "hurricane", "smog", "meteor", "rainbow",
                    "umbrella", "droplet", "temperature", "water"))],
        "wi": sorted(WI_ICONS.keys()),
        "nature": [k for k in FA_ICONS if k.startswith(("fire", "leaf", "tree", "seed",
                   "mountain", "fish", "dove", "feather", "paw", "bug", "spider",
                   "worm", "frog", "crow", "dragon", "hippo", "horse", "otter",
                   "dog", "cat", "kiwi", "locust", "shrimp"))],
        "arrows": [k for k in FA_ICONS if any(w in k for w in
                   ("arrow", "chevron", "angle", "location", "compass",
                    "route", "map", "sign"))],
        "status": [k for k in FA_ICONS if any(w in k for w in
                   ("check", "xmark", "circle-check", "circle-x", "circle-info",
                    "circle-excl", "circle-quest", "triangle", "thumb", "flag",
                    "bookmark", "ban", "shield", "lock", "unlock", "key",
                    "fingerprint", "radiation", "biohazard", "skull"))],
        "ui": [k for k in FA_ICONS if any(w in k for w in
               ("bars", "ellipsis", "gear", "slider", "magnif", "eye", "pen",
                "pencil", "eraser", "trash", "plus", "minus", "toggle",
                "filter", "sort", "grip", "expand", "compress", "max", "min",
                "crop", "scissor", "copy", "paste", "clipboard"))],
        "shapes": [k for k in FA_ICONS if k in
                   ("heart", "star", "star-half", "circle", "square", "diamond",
                    "play", "infinity", "hashtag", "at", "percent", "bolt",
                    "crown", "cross", "certificate")],
        "objects": [k for k in FA_ICONS if any(w in k for w in
                    ("house", "building", "store", "bell", "light", "plug",
                     "battery", "wrench", "hammer", "screwdriver", "toolbox",
                     "briefcase", "suitcase", "box", "gift", "trophy", "medal",
                     "award", "graduation", "book", "newspaper", "calendar",
                     "clock", "hourglass", "stopwatch", "utensil", "mug",
                     "wine", "martini", "beer", "pizza", "cookie", "apple",
                     "carrot", "lemon", "candy", "ice-cream", "cake"))],
        "people": [k for k in FA_ICONS if any(w in k for w in
                   ("user", "person", "child", "baby", "hand", "shake",
                    "face-", "skull", "ghost"))],
        "tech": [k for k in FA_ICONS if any(w in k for w in
                 ("wifi", "signal", "satellite", "tower", "laptop", "desktop",
                  "mobile", "tablet", "keyboard", "print", "camera", "video",
                  "microphone", "headphone", "tv", "server", "hard-drive",
                  "microchip", "robot", "code", "terminal", "database",
                  "qrcode", "barcode", "power"))],
        "communication": [k for k in FA_ICONS if any(w in k for w in
                          ("phone", "envelope", "inbox", "comment", "message",
                           "paper-plane", "share", "rss", "bullhorn"))],
        "charts": [k for k in FA_ICONS if any(w in k for w in
                   ("chart", "table", "list", "gauge"))],
        "files": [k for k in FA_ICONS if any(w in k for w in
                  ("file", "folder", "download", "upload", "cloud-arrow",
                   "floppy"))],
        "media": [k for k in FA_ICONS if k in
                  ("pause", "stop", "forward", "backward", "forward-step",
                   "backward-step", "forward-fast", "backward-fast",
                   "shuffle", "repeat", "volume-high", "volume-low",
                   "volume-off", "volume-xmark", "music", "image", "film",
                   "palette", "paintbrush", "spray-can")],
        "transport": [k for k in FA_ICONS if any(w in k for w in
                      ("car", "truck", "bus", "taxi", "motorcycle", "bicycle",
                       "plane", "helicopter", "ship", "sailboat", "train",
                       "rocket", "shuttle", "gas-pump", "road", "trailer"))],
        "health": [k for k in FA_ICONS if any(w in k for w in
                   ("heart-pulse", "stethoscope", "syringe", "pill", "capsule",
                    "vial", "microscope", "flask", "atom", "dna", "virus",
                    "lung", "brain", "bone", "tooth", "hospital", "bandage",
                    "wheelchair"))],
        "commerce": [k for k in FA_ICONS if any(w in k for w in
                     ("cart", "bag-shop", "basket", "credit", "money", "coin",
                      "wallet", "receipt", "cash", "tag"))],
    }
    groups["aliases"] = sorted(ICON_ALIASES.keys())
    groups["all"] = sorted(FA_ICONS.keys())

    cat = category.lower().strip()
    if not cat or cat not in groups:
        summary = {c: len(v) for c, v in groups.items() if c != "all"}
        total_fa = len(FA_ICONS)
        total_wi = len(WI_ICONS)
        total_alias = len(ICON_ALIASES)
        return (
            f"Icon catalog: {total_fa} FontAwesome + {total_wi} Weather Icons "
            f"+ {total_alias} aliases.\n"
            f"Categories: {', '.join(f'{c} ({n})' for c, n in summary.items())}\n"
            f"Call list_icons(category='weather') etc. to see names."
        )

    icons = groups[cat]
    return f"{cat} ({len(icons)} icons): {', '.join(icons)}"


if __name__ == "__main__":
    mcp.run()
