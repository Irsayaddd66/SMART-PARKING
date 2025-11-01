# Sistem Parkir Pintar IoT 3-Slot (ESP32 & Blynk)

Sebuah prototipe sistem parkir pintar yang mampu memonitor 3 slot parkir secara independen. Proyek ini menggunakan satu ESP32 untuk mengelola 3 sensor ultrasonik, 3 layar LCD terpisah, dan 6 output (LED/Buzzer) melalui *shift register*, serta terhubung ke *dashboard* Blynk untuk monitoring *real-time*.

![[Masukkan Screenshot Proyek/Demo GIF Anda Di Sini]](demo.gif)

---

## 🚀 Fitur Utama

* **Deteksi 3-Slot Independen:** 3 sensor HC-SR04 memantau setiap slot secara individual.
* **Tampilan Informasi Unik:** 3 layar LCD ILI9341 didedikasikan untuk setiap slot, menampilkan:
    * Status: `TERISI` (Merah) atau `KOSONG` (Hijau).
    * Durasi Parkir: *Timer* yang berjalan (disimulasikan 10 detik = 1 jam).
    * Biaya Parkir: Kalkulator biaya otomatis (Rp 10.000 per 5 jam virtual).
* **Indikator Fisik:** 3 LED dan 3 Buzzer memberikan *feedback* instan saat mobil masuk, dikontrol oleh satu chip 74HC595.
* **Monitoring IoT:** Terhubung ke **Blynk Cloud**, mengirimkan status `ON/OFF` ke widget LED virtual (V0, V1, V2) secara *real-time*.

---

## 🤯 Tantangan Teknis & Solusi

Tantangan utama proyek ini adalah keterbatasan pin GPIO pada ESP32 untuk mengontrol 12+ komponen.

**Solusi:** Implementasi **SPI Bus Sharing**.

* **Jalan Tol Bersama (Shared Bus):** Pin `MOSI` (GPIO 13) dan `SCK` (GPIO 14) digunakan **bersama-sama** oleh **4 perangkat** (3 LCD + 1 74HC595).
* **Kunci Pintu Unik (Enable Pins):** ESP32 menggunakan 4 pin GPIO terpisah untuk bertindak sebagai "Kunci" atau "Saklar" untuk memilih perangkat mana yang akan diajak bicara:
    1.  `PIN_LCD1_CS` (GPIO 15) -> "Kunci Pintu" untuk **LCD 1**.
    2.  `PIN_LCD2_CS` (GPIO 25) -> "Kunci Pintu" untuk **LCD 2**.
    3.  `PIN_LCD3_CS` (GPIO 32) -> "Kunci Pintu" untuk **LCD 3**.
    4.  `PIN_LATCH` (GPIO 16) -> "Kunci Pintu" (Latch) untuk **74HC595**.

---

## 🛠️ Komponen

| Komponen | Jumlah | Fungsi |
| :--- | :--- | :--- |
| ESP32 WROOM 32 | 1 | Otak utama, WiFi, & Kontroler |
| LCD ILI9341 (SPI) | 3 | Penampil Status, Durasi, Biaya |
| Sensor HC-SR04 | 3 | Detektor Kendaraan |
| 74HC595 Shift Register | 1 | I/O Expander (Menambah Pin Output) |
| LED Merah | 3 | Indikator Fisik "TERISI" |
| Buzzer Aktif | 3 | Indikator Audio "Mobil Masuk" |

---

## ⚙️ Cara Kerja Alur Program

1.  **`void setup()`**:
    * Menginisialisasi Serial Monitor, semua pin I/O (Sensor, Latch), dan bus `SPI`.
    * Memulai (`.begin()`) dan memutar (`.setRotation()`) ketiga LCD.
    * Menggambar tampilan awal ("KOSONG" hijau) di ketiga LCD.
    * Menghubungkan ke WiFi dan Blynk (`Blynk.begin()`).
    * Melakukan sinkronisasi awal: mengirim `0` (OFF) ke semua Virtual Pin Blynk dan `kirimKeShiftRegister(0)` untuk mematikan semua LED/Buzzer.

2.  **`void loop()`**:
    * **`Blynk.run()`**: Perintah wajib untuk menjaga koneksi ke server Blynk.
    * **`prosesSlot1/2/3()`**: Memanggil 3 fungsi "otak" untuk setiap slot secara independen.
    * **`kirimKeShiftRegister(dataOutput)`**: Mengirim status 8-bit terbaru (dari variabel `dataOutput`) ke 74HC595 di **setiap putaran loop** untuk mengupdate LED/Buzzer.
    * **Timer 500ms:** Setiap 500ms, program memperbarui tampilan Durasi dan Biaya di ketiga LCD.
    * **Timer 2000ms:** Setiap 2 detik, program mengirim data tambahan (biaya, jarak, dll.) ke Virtual Pin Blynk lainnya (V3-V11).

3.  **Fungsi Kunci: `prosesSlot1()`**:
    * **Deteksi Perubahan (Edge Detection):** Logika `if (terdeteksi && !statusTerisi1)` hanya berjalan **satu kali** saat mobil *baru masuk*.
    * **Aksi Masuk:** Mengatur `statusTerisi = true`, memulai `waktuMulai`, dan `bitSet()` (menyalakan) bit LED & Buzzer di `dataOutput`, lalu mengirim `Blynk.virtualWrite(..., 1)`.
    * **Aksi Keluar:** Logika `else if (!terdeteksi && statusTerisi1)` hanya berjalan **satu kali** saat mobil *baru keluar*.
    * **Timer Buzzer:** Logika `if (statusBuzzerAktif1 && ...)` akan `bitClear()` (mematikan) bit Buzzer setelah 150ms, sementara bit LED tetap `ON`.

---

## 🔌 Instalasi

1.  Clone repositori ini.
2.  Buka proyek di Arduino IDE atau Wokwi.
3.  Pastikan library `Blynk`, `Adafruit_GFX`, dan `Adafruit_ILI9341` terinstal.
4.  **Konfigurasi Wajib:** Buka file `.ino` dan masukkan kredensial Anda di bagian paling atas:
    ```cpp
    #define BLYNK_TEMPLATE_ID "TEMPLATE_ID_ANDA"
    #define BLYNK_TEMPLATE_NAME "NAMA_TEMPLATE_ANDA"
    #define BLYNK_AUTH_TOKEN "TOKEN_ANDA"
    
    const char* ssid = "NAMA_WIFI_ANDA";
    const char* password = "PASSWORD_WIFI_ANDA";
    ```
5.  Konfigurasi Datastream di Blynk Anda agar sesuai (V0, V1, V2 untuk LED).
6.  Upload dan jalankan!