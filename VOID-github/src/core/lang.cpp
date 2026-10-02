#include "lang.hpp"

namespace lang
{
    namespace
    {
        Id g_lang = EN;

        const char* g_en[] =
        {
            "Secure Loader", "Premium System Optimizer", "LICENSE KEY", "Generate", "Remember me",
            "Activate license", "Demo mode", "any key is accepted", "Contacting license server...",
            "Please enter your license key", "Activation failed", "Signed out", "Your session has been closed",

            "Dashboard", "Cleaner", "Tweaks", "Network", "Settings", "System Info",
            "Overview of your system at a glance",
            "Find and remove junk files to reclaim space",
            "Fine-tune Windows for performance and privacy",
            "Lower latency and optimize your connection",
            "Personalize the look and behaviour of VOID",
            "Detailed hardware and software information",

            "Processor", "Memory", "Storage", "Health", "Excellent", "Good", "Needs attention",
            "%d logical threads", "of %.0f GB in use", "free of %.0f GB on C:",
            "Processor load", "Live \xC2\xB7 last 30 seconds", "Quick optimize", "One click, every module",
            "health score", "optimizing", "Optimize now",
            "System optimized", "Memory trimmed, caches flushed",

            "System", "Subscription", "Operating system", "Graphics", "Computer", "User", "Uptime",
            "Plan", "Status", "Active", "Expires", "HWID",

            "Ready to scan", "Select categories below and start a scan.",
            "Scan complete", "Junk found", "ready to clean",
            "All clean", "your system is spotless", "freed", "Scan", "Rescan", "Clean now", "CATEGORIES",

            "Temporary files", "Browser cache", "Windows Update cache", "Recycle Bin", "Prefetch data",
            "System logs", "Thumbnail cache", "Crash dumps", "Shader cache", "Delivery Optimization",
            "User and system temp folders", "Chrome, Edge, Firefox, Opera",
            "Downloaded update packages", "Deleted files waiting to go",
            "Application launch traces", "Event and setup log files",
            "Explorer preview database", "Memory dumps and error reports",
            "DirectX and driver shaders", "Peer-to-peer update cache",

            "Performance", "Gaming", "Privacy", "Visual", "Games", "FiveM", "Delay",
            "Apply", "Reset", "Defaults restored", "Tweaks applied", "tweaks active",
            "need admin", "restart recommended",

            "RAM optimization", "Group services into fewer svchost processes",
            "Installed memory", "detected", "Current profile",
            "Apply profile", "RAM profile applied", "Restart required to take effect",
            "Custom", "recommended",

            "Latency", "Round-trip time", "simulated", "jitter",
            "DNS provider", "Resolver used by your network adapters", "Flush DNS cache",
            "DNS updated", "DNS cache flushed",
            "Connection", "Active network adapter details",
            "Automatic", "OPTIMIZATIONS",

            "Visual effects", "Tune the background scene",
            "Particles", "Connection lines", "Mouse interaction",
            "Top light", "Light sweep", "Particle count", "Particle speed",
            "General", "Application behaviour", "Remember license", "Notifications", "Show toast messages",
            "Launch at startup", "Minimize to tray", "Account", "Sign out",
            "Language",

            "Hardware", "Software", "Security", "Network",
            "CPU", "Cores", "Threads", "Base clock",
            "GPU", "VRAM", "Driver",
            "Total RAM", "Speed", "Slots",
            "Motherboard", "BIOS version", "BIOS mode",
            "Secure Boot", "Enabled", "Disabled", "Not supported",
            "Virtualization", "Install date", "DirectX",
            "Display", "System locale",
        };

        // Turkish - split hex escapes from following a-f chars with string concat
        const char* g_tr[] =
        {
            "G\xC3\xBC" "venli Y\xC3\xBC" "kleyici", "Premium Sistem Optimize Edici",
            "L\xC4\xB0SANS ANAHTARI", "Olu\xC5\x9Ftur", "Beni hat\xC4\xB1rla",
            "Lisans\xC4\xB1 etkinle\xC5\x9Ftir", "Demo modu",
            "herhangi bir anahtar kabul edilir",
            "Lisans sunucusuna ba\xC4\x9Flan\xC4\xB1l\xC4\xB1yor...",
            "L\xC3\xBCtfen lisans anahtar\xC4\xB1n\xC4\xB1z\xC4\xB1 girin",
            "Etkinle\xC5\x9Ftirme ba\xC5\x9F" "ar\xC4\xB1s\xC4\xB1z",
            "\xC3\x87\xC4\xB1k\xC4\xB1\xC5\x9F yap\xC4\xB1ld\xC4\xB1",
            "Oturumunuz kapat\xC4\xB1ld\xC4\xB1",

            "G\xC3\xB6sterge Paneli", "Temizleyici", "\xC4\xB0nce Ayarlar",
            "A\xC4\x9F", "Ayarlar", "Sistem Bilgisi",
            "Sisteminize genel bir bak\xC4\xB1\xC5\x9F",
            "Gereksiz dosyalar\xC4\xB1 bulup temizleyin",
            "Windows'\xC4\xB1 performans i\xC3\xA7in optimize edin",
            "Gecikmeyi d\xC3\xBC\xC5\x9F\xC3\xBC" "r\xC3\xBC" "n, ba\xC4\x9Flant\xC4\xB1y\xC4\xB1 optimize edin",
            "VOID'\xC4\xB1n g\xC3\xB6r\xC3\xBC" "n\xC3\xBC" "m ve davran\xC4\xB1\xC5\x9F\xC4\xB1n\xC4\xB1 \xC3\xB6" "zelle\xC5\x9Ftirin",
            "Detayl\xC4\xB1 donan\xC4\xB1m ve yaz\xC4\xB1l\xC4\xB1m bilgisi",

            "\xC4\xB0\xC5\x9Flemci", "Bellek", "Depolama", "Sa\xC4\x9Fl\xC4\xB1k",
            "M\xC3\xBC" "kemmel", "\xC4\xB0yi", "\xC4\xB0lgiye ihtiya\xC3\xA7 var",
            "%d mant\xC4\xB1ksal i\xC5\x9F par\xC3\xA7" "a" "c\xC4\xB1\xC4\x9F\xC4\xB1",
            "%.0f GB kullan\xC4\xB1mda", "C: \xC3\xBC" "zerinde %.0f GB bo\xC5\x9F",
            "\xC4\xB0\xC5\x9Flemci y\xC3\xBC" "k\xC3\xBC",
            "Canl\xC4\xB1 \xC2\xB7 son 30 saniye",
            "H\xC4\xB1zl\xC4\xB1 optimizasyon", "Tek t\xC4\xB1kla, t\xC3\xBC" "m mod\xC3\xBC" "ller",
            "sa\xC4\x9Fl\xC4\xB1k puan\xC4\xB1", "optimize ediliyor", "\xC5\x9Eimdi optimize et",
            "Sistem optimize edildi", "Bellek temizlendi, \xC3\xB6nbellekler bo\xC5\x9F" "alt\xC4\xB1ld\xC4\xB1",

            "Sistem", "Abonelik", "\xC4\xB0\xC5\x9Fletim sistemi", "Grafik", "Bilgisayar",
            "Kullan\xC4\xB1" "c\xC4\xB1", "\xC3\x87" "al\xC4\xB1\xC5\x9Fma s\xC3\xBCresi",
            "Plan", "Durum", "Aktif", "Biti\xC5\x9F", "HWID",

            "Taramaya haz\xC4\xB1r", "A\xC5\x9F" "a\xC4\x9F\xC4\xB1" "dan kategorileri se\xC3\xA7ip taray\xC4\xB1n.",
            "Tarama tamamland\xC4\xB1", "Gereksiz dosya bulundu", "temizlemeye haz\xC4\xB1r",
            "Temiz", "sisteminiz tertemiz", "temizlendi",
            "Tara", "Yeniden tara", "\xC5\x9Eimdi temizle", "KATEGOR\xC4\xB0LER",

            "Ge\xC3\xA7ici dosyalar", "Taray\xC4\xB1" "c\xC4\xB1 \xC3\xB6nbelle\xC4\x9Fi",
            "Windows Update \xC3\xB6nbelle\xC4\x9Fi", "Geri D\xC3\xB6n\xC3\xBC\xC5\x9F\xC3\xBCm Kutusu",
            "\xC3\x96ny\xC3\xBC" "kleme verileri", "Sistem g\xC3\xBC" "nl\xC3\xBC" "kleri",
            "K\xC3\xBC\xC3\xA7\xC3\xBC" "k resim \xC3\xB6nbelle\xC4\x9Fi",
            "\xC3\x87\xC3\xB6kme d\xC3\xB6k\xC3\xBCmleri", "G\xC3\xB6lgelendiri" "ci \xC3\xB6nbelle\xC4\x9Fi",
            "Teslim Optimizasyonu",
            "Kullan\xC4\xB1" "c\xC4\xB1 ve sistem ge\xC3\xA7i" "ci klas\xC3\xB6rleri",
            "Chrome, Edge, Firefox, Opera",
            "\xC4\xB0ndirilen g\xC3\xBC" "n" "celleme paketleri",
            "Silinmeyi bekleyen dosyalar",
            "Uygulama ba\xC5\x9Flatma izleri",
            "Olay ve kurulum g\xC3\xBC" "nl\xC3\xBC" "k dosyalar\xC4\xB1",
            "Gezgin \xC3\xB6nizleme veritaban\xC4\xB1",
            "Bellek d\xC3\xB6k\xC3\xBCmleri ve hata raporlar\xC4\xB1",
            "DirectX ve s\xC3\xBC" "r\xC3\xBC" "c\xC3\xBC g\xC3\xB6lgelendiri" "cileri",
            "E\xC5\x9Fler aras\xC4\xB1 g\xC3\xBC" "n" "celleme \xC3\xB6nbelle\xC4\x9Fi",

            "Performans", "Oyun", "Gizlilik", "G\xC3\xB6rsel", "Oyunlar", "FiveM", "Gecikme",
            "Uygula", "S\xC4\xB1" "f\xC4\xB1rla",
            "Varsay\xC4\xB1lanlar geri y\xC3\xBC" "klendi",
            "\xC4\xB0n" "ce ayarlar uyguland\xC4\xB1",
            "in" "ce ayar aktif", "y\xC3\xB6neti" "ci gerekli",
            "yeniden ba\xC5\x9Flatma \xC3\xB6nerilir",

            "RAM optimizasyonu", "Servisleri daha az svchost i\xC5\x9Flemine grupla",
            "Kurulu bellek", "alg\xC4\xB1land\xC4\xB1", "Ge\xC3\xA7" "erli profil",
            "Profili uygula", "RAM profili uyguland\xC4\xB1",
            "Etkili olmas\xC4\xB1 i\xC3\xA7in yeniden ba\xC5\x9Flatma gerekir",
            "\xC3\x96zel", "\xC3\xB6nerilen",

            "Gecikme", "Gidi\xC5\x9F-d\xC3\xB6n\xC3\xBC\xC5\x9F s\xC3\xBCresi",
            "sim\xC3\xBCle", "titreme",
            "DNS sa\xC4\x9Flay\xC4\xB1" "c\xC4\xB1s\xC4\xB1",
            "A\xC4\x9F" " " "a" "dapt\xC3\xB6rlerinizin kulland\xC4\xB1\xC4\x9F\xC4\xB1 \xC3\xA7\xC3\xB6z\xC3\xBCmleyici",
            "DNS \xC3\xB6nbelle\xC4\x9Fini temizle",
            "DNS g\xC3\xBC" "n" "cellendi", "DNS \xC3\xB6nbelle\xC4\x9Fi temizlendi",
            "Ba\xC4\x9Flant\xC4\xB1",
            "Aktif a\xC4\x9F" " " "a" "dapt\xC3\xB6r\xC3\xBC detaylar\xC4\xB1",
            "Otomatik", "OPT\xC4\xB0M\xC4\xB0ZASYONLAR",

            "G\xC3\xB6rsel efektler", "Arka plan sahnesini ayarla",
            "Par\xC3\xA7" "a" "c\xC4\xB1klar", "Ba\xC4\x9Flant\xC4\xB1 \xC3\xA7izgileri",
            "Fare etkile\xC5\x9Fimi",
            "\xC3\x9Cst \xC4\xB1\xC5\x9F\xC4\xB1k", "I\xC5\x9F\xC4\xB1k tarama",
            "Par\xC3\xA7" "a" "c\xC4\xB1k say\xC4\xB1s\xC4\xB1", "Par\xC3\xA7" "a" "c\xC4\xB1k h\xC4\xB1z\xC4\xB1",
            "Genel", "Uygulama davran\xC4\xB1\xC5\x9F\xC4\xB1",
            "Lisans\xC4\xB1 hat\xC4\xB1rla", "Bildirimler", "Bildirim mesajlar\xC4\xB1",
            "Ba\xC5\x9Flang\xC4\xB1" "c" "ta \xC3\xA7" "al\xC4\xB1\xC5\x9Ft\xC4\xB1r",
            "Tepsi simgesine k\xC3\xBC\xC3\xA7\xC3\xBC" "lt",
            "Hesap", "\xC3\x87\xC4\xB1k\xC4\xB1\xC5\x9F yap",
            "Dil",

            "Donan\xC4\xB1m", "Yaz\xC4\xB1l\xC4\xB1m", "G\xC3\xBC" "venlik", "A\xC4\x9F",
            "\xC4\xB0\xC5\x9Flemci", "\xC3\x87" "ekirdekler", "\xC4\xB0\xC5\x9F Par\xC3\xA7" "a" "c\xC4\xB1klar\xC4\xB1",
            "Temel saat h\xC4\xB1z\xC4\xB1",
            "Ekran kart\xC4\xB1", "VRAM", "S\xC3\xBC" "r\xC3\xBC" "c\xC3\xBC",
            "Toplam RAM", "H\xC4\xB1z", "Yuvalar",
            "Anakart", "BIOS s\xC3\xBC" "r\xC3\xBCm\xC3\xBC", "BIOS modu",
            "G\xC3\xBC" "venli \xC3\x96ny\xC3\xBC" "kleme",
            "Etkin", "Devre d\xC4\xB1\xC5\x9F\xC4\xB1", "Desteklenmiyor",
            "Sanalla\xC5\x9Ft\xC4\xB1rma", "Kurulum tarihi", "DirectX",
            "Ekran \xC3\xA7\xC3\xB6z\xC3\xBC" "n\xC3\xBCrl\xC3\xBC\xC4\x9F\xC3\xBC",
            "Sistem dili",
        };
    }

    void Set(Id id)   { g_lang = id; }
    Id   Current()    { return g_lang; }

    const char* Get(int key)
    {
        if (key < 0 || key >= S::_COUNT) return "???";
        return g_lang == TR ? g_tr[key] : g_en[key];
    }
}
