#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ==========================================================
// == KONFIGURASI BLYNK ==
// ==========================================================
#define BLYNK_TEMPLATE_ID "TMPL6Pov5cn-X"
#define BLYNK_TEMPLATE_NAME "SmartParking"
#define BLYNK_AUTH_TOKEN "AmEJGv7mkoYjumeVhlOvQCBZ60PNdTbK"
#define BLYNK_PRINT Serial 
#include <BlynkSimpleEsp32.h>

// --- Virtual Pin untuk Datastream Blynk ---
#define BLYNK_VIRTUAL_PIN_LED1 V0
#define BLYNK_VIRTUAL_PIN_LED2 V1
#define BLYNK_VIRTUAL_PIN_LED3 V2

// ==========================================================
// == KONFIGURASI JARINGAN ==
// ==========================================================
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ==========================================================
// == DEFINISI PIN
// ==========================================================

// --- Pin SPI (Bersama) ---
#define PIN_SPI_MOSI 13 // (DS di 74HC595)
#define PIN_SPI_MISO 12
#define PIN_SPI_SCK  14 // (SH_CP di 74HC595)

// --- Pin Latch 74HC595 ---
#define PIN_LATCH 16 // (ST_CP di 74HC595)

// --- Pin LCD 1 ---
#define PIN_LCD1_CS   15
#define PIN_LCD1_RST  21
#define PIN_LCD1_DC   2

// --- Pin LCD 2 ---
#define PIN_LCD2_CS   25
#define PIN_LCD2_RST  27
#define PIN_LCD2_DC   26

// --- Pin LCD 3 ---
#define PIN_LCD3_CS   32
#define PIN_LCD3_RST  17
#define PIN_LCD3_DC   33

// --- Pin Sensor HC-SR04 ---
#define PIN_TRIG1 4
#define PIN_ECHO1 5
#define PIN_TRIG2 18
#define PIN_ECHO2 19
#define PIN_TRIG3 22
#define PIN_ECHO3 23

// --- Definisi Bit di 74HC595 (Q0 s/d Q7) ---
#define BIT_LED1    0
#define BIT_LED2    1
#define BIT_LED3    2
#define BIT_BUZZER1 3
#define BIT_BUZZER2 4
#define BIT_BUZZER3 5

// ==========================================================
// == PENGATURAN LOGIKA ==
// ==========================================================
const int BATAS_JARAK_DETEKSI_CM = 30; // Jarak deteksi mobil (cm)
const long DURASI_BIP_MS = 150; // Durasi buzzer bunyi (ms)

// Simulasi Waktu: 1 Jam Virtual = 10 Detik Nyata
const long SATU_JAM_VIRTUAL_MS = 10000; 

// ==========================================================
// == INISIALISASI OBJEK ==
// ==========================================================
Adafruit_ILI9341 lcd1 = Adafruit_ILI9341(PIN_LCD1_CS, PIN_LCD1_DC, PIN_LCD1_RST);
Adafruit_ILI9341 lcd2 = Adafruit_ILI9341(PIN_LCD2_CS, PIN_LCD2_DC, PIN_LCD2_RST);
Adafruit_ILI9341 lcd3 = Adafruit_ILI9341(PIN_LCD3_CS, PIN_LCD3_DC, PIN_LCD3_RST);

// ==========================================================
// == VARIABEL GLOBAL UNTUK LOGIKA ==
// ==========================================================
byte dataOutput = 0; // Menyimpan status 8-bit untuk 74HC595

// --- Status untuk Slot 1 ---
bool statusTerisi1 = false;
unsigned long waktuMulai1 = 0;
bool statusBuzzerAktif1 = false;
String statusSebelumnya1 = "";
unsigned long waktuUpdateBlynk1 = 0;

// --- Status untuk Slot 2 ---
bool statusTerisi2 = false;
unsigned long waktuMulai2 = 0;
bool statusBuzzerAktif2 = false;
String statusSebelumnya2 = "";
unsigned long waktuUpdateBlynk2 = 0;

// --- Status untuk Slot 3 ---
bool statusTerisi3 = false;
unsigned long waktuMulai3 = 0;
bool statusBuzzerAktif3 = false;
String statusSebelumnya3 = "";
unsigned long waktuUpdateBlynk3 = 0;

// --- Timer untuk update layar ---
unsigned long waktuUpdateTerakhir = 0;
bool statusKoneksiBlynk = false;

// ==========================================================
// == FUNGSI SETUP ==
// ==========================================================
void setup() {
  Serial.begin(115200);
  Serial.println(" Memulai Sistem Parkir Pintar...");

  // Inisialisasi Pin Sensor
  pinMode(PIN_TRIG1, OUTPUT); pinMode(PIN_ECHO1, INPUT);
  pinMode(PIN_TRIG2, OUTPUT); pinMode(PIN_ECHO2, INPUT);
  pinMode(PIN_TRIG3, OUTPUT); pinMode(PIN_ECHO3, INPUT);
  Serial.println(" Sensor HC-SR04 siap");

  // Inisialisasi Pin Latch 74HC595
  pinMode(PIN_LATCH, OUTPUT);
  digitalWrite(PIN_LATCH, HIGH);
  Serial.println(" 74HC595 siap");

  // Inisialisasi SPI
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  Serial.println(" Bus SPI siap");

  // Inisialisasi 3 LCD
  Serial.println(" Menginisialisasi Layar...");
  lcd1.begin(); lcd1.setRotation(1);
  lcd2.begin(); lcd2.setRotation(1);
  lcd3.begin(); lcd3.setRotation(1);
  
  // Fungsi Gambar tampilan awal sekali saja
  gambarTampilanAwal(lcd1, "SLOT PARKIR 1");
  gambarTampilanAwal(lcd2, "SLOT PARKIR 2"); 
  gambarTampilanAwal(lcd3, "SLOT PARKIR 3");
  
  Serial.println(" LCD 1, 2, 3 siap");

  // Koneksi ke Blynk
  Serial.println("Menghubungkan ke Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  
  // Tunggu koneksi Blynk
  int timeout = 0;
  while (!Blynk.connected() && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  
  if (Blynk.connected()) {
    statusKoneksiBlynk = true;
    Serial.println("\n Terhubung ke Blynk!");
    
    // Kirim status awal ke Blynk
    Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_LED1, 0);
    Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_LED2, 0);
    Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_LED3, 0);
  } else {
    Serial.println("\n Gagal terhubung ke Blynk");
  }

  // Pastikan semua output mati
  kirimKeShiftRegister(0);
  Serial.println(" Setup Selesai. Sistem berjalan.");
}

// ==========================================================
// == FUNGSI LOOP UTAMA ==
// ==========================================================
void loop() {
  unsigned long waktuSekarang = millis();

  // Jalankan Blynk
  Blynk.run();

  // --- 1. PROSES LOGIKA SENSOR & OUTPUT ---
  prosesSlot1(waktuSekarang);
  prosesSlot2(waktuSekarang);
  prosesSlot3(waktuSekarang);

  // Kirim update status (LED/Buzzer) ke 74HC595
  kirimKeShiftRegister(dataOutput);

  // --- 2. PROSES UPDATE LAYAR (Setiap 500ms) ---
  if (waktuSekarang - waktuUpdateTerakhir > 500) {
    waktuUpdateTerakhir = waktuSekarang;

    // Hitung durasi
    unsigned long durasi1 = (statusTerisi1) ? (waktuSekarang - waktuMulai1) : 0;
    unsigned long durasi2 = (statusTerisi2) ? (waktuSekarang - waktuMulai2) : 0;
    unsigned long durasi3 = (statusTerisi3) ? (waktuSekarang - waktuMulai3) : 0;

    // Update hanya bagian yang berubah
    perbaruiTampilanSlot(lcd1, 1, statusTerisi1, durasi1);
    perbaruiTampilanSlot(lcd2, 2, statusTerisi2, durasi2);
    perbaruiTampilanSlot(lcd3, 3, statusTerisi3, durasi3);
    
    // Update Blynk setiap 2 detik
    if (statusKoneksiBlynk) {
      if (waktuSekarang - waktuUpdateBlynk1 > 2000) {
        kirimDataKeBlynk();
        waktuUpdateBlynk1 = waktuSekarang;
      }
    }
    
    // Debug serial
    Serial.printf("Jarak: [S1:%ldcm] [S2:%ldcm] [S3:%ldcm] | Blynk: %s\n", 
                  bacaJarak(PIN_TRIG1, PIN_ECHO1), 
                  bacaJarak(PIN_TRIG2, PIN_ECHO2), 
                  bacaJarak(PIN_TRIG3, PIN_ECHO3),
                  statusKoneksiBlynk ? "OK" : "OFF");
  }
}

// ==========================================================
// == FUNGSI LOGIKA PER SLOT ==
// ==========================================================

void prosesSlot1(unsigned long waktuSekarang) {
  long jarak = bacaJarak(PIN_TRIG1, PIN_ECHO1);
  bool terdeteksi = (jarak < BATAS_JARAK_DETEKSI_CM && jarak > 0);

  // Mobil BARU TIBA
  if (terdeteksi && !statusTerisi1) {
    statusTerisi1 = true;
    waktuMulai1 = waktuSekarang;
    statusBuzzerAktif1 = true;
    bitSet(dataOutput, BIT_LED1);    // Nyalakan LED
    bitSet(dataOutput, BIT_BUZZER1); // Nyalakan Buzzer
    
    // Update Blynk
    if (statusKoneksiBlynk) {
      Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_LED1, 1);
    }
    
    Serial.println("Slot 1: Mobil masuk");
  }
  // Mobil PERGI
  else if (!terdeteksi && statusTerisi1) {
    statusTerisi1 = false;
    waktuMulai1 = 0;
    bitClear(dataOutput, BIT_LED1); // Matikan LED
    
    // Update Blynk segera
    if (statusKoneksiBlynk) {
      Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_LED1, 0);
    }
    
    Serial.println("Slot 1: Mobil keluar");
  }

  // Matikan buzzer setelah durasi singkat
  if (statusBuzzerAktif1 && (waktuSekarang - waktuMulai1 > DURASI_BIP_MS)) {
    statusBuzzerAktif1 = false;
    bitClear(dataOutput, BIT_BUZZER1);
  }
}

void prosesSlot2(unsigned long waktuSekarang) {
  long jarak = bacaJarak(PIN_TRIG2, PIN_ECHO2);
  bool terdeteksi = (jarak < BATAS_JARAK_DETEKSI_CM && jarak > 0);

  if (terdeteksi && !statusTerisi2) {
    statusTerisi2 = true;
    waktuMulai2 = waktuSekarang;
    statusBuzzerAktif2 = true;
    bitSet(dataOutput, BIT_LED2);
    bitSet(dataOutput, BIT_BUZZER2);
    
    if (statusKoneksiBlynk) {
      Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_LED2, 1);
    }
    
    Serial.println("Slot 2: Mobil masuk");
  } else if (!terdeteksi && statusTerisi2) {
    statusTerisi2 = false;
    waktuMulai2 = 0;
    bitClear(dataOutput, BIT_LED2);
    
    if (statusKoneksiBlynk) {
      Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_LED2, 0);
    }
    
    Serial.println("Slot 2: Mobil keluar");
  }
  if (statusBuzzerAktif2 && (waktuSekarang - waktuMulai2 > DURASI_BIP_MS)) {
    statusBuzzerAktif2 = false;
    bitClear(dataOutput, BIT_BUZZER2);
  }
}

void prosesSlot3(unsigned long waktuSekarang) {
  long jarak = bacaJarak(PIN_TRIG3, PIN_ECHO3);
  bool terdeteksi = (jarak < BATAS_JARAK_DETEKSI_CM && jarak > 0);

  if (terdeteksi && !statusTerisi3) {
    statusTerisi3 = true;
    waktuMulai3 = waktuSekarang;
    statusBuzzerAktif3 = true;
    bitSet(dataOutput, BIT_LED3);
    bitSet(dataOutput, BIT_BUZZER3);
    
    if (statusKoneksiBlynk) {
      Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_LED3, 1);
    }
    
    Serial.println("🚗 Slot 3: Mobil masuk");
  } else if (!terdeteksi && statusTerisi3) {
    statusTerisi3 = false;
    waktuMulai3 = 0;
    bitClear(dataOutput, BIT_LED3);
    
    if (statusKoneksiBlynk) {
      Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_LED3, 0);
    }
    
    Serial.println("🅿️ Slot 3: Mobil keluar");
  }
  if (statusBuzzerAktif3 && (waktuSekarang - waktuMulai3 > DURASI_BIP_MS)) {
    statusBuzzerAktif3 = false;
    bitClear(dataOutput, BIT_BUZZER3);
  }
}

// ==========================================================
// == FUNGSI BLYNK ==
// ==========================================================

/**
 * Kirim data lengkap ke Blynk
 */
void kirimDataKeBlynk() {
  if (!statusKoneksiBlynk) return;
  
  // Hitung durasi dan biaya
  unsigned long waktuSekarang = millis();
  unsigned long durasi1 = statusTerisi1 ? (waktuSekarang - waktuMulai1) : 0;
  unsigned long durasi2 = statusTerisi2 ? (waktuSekarang - waktuMulai2) : 0;
  unsigned long durasi3 = statusTerisi3 ? (waktuSekarang - waktuMulai3) : 0;
  
  int biaya1 = hitungBiaya(durasi1);
  int biaya2 = hitungBiaya(durasi2);
  int biaya3 = hitungBiaya(durasi3);
  
  // Kirim notifikasi status
  Blynk.virtualWrite(V3, statusTerisi1 ? 1 : 0); // Status slot 1
  Blynk.virtualWrite(V4, statusTerisi2 ? 1 : 0); // Status slot 2
  Blynk.virtualWrite(V5, statusTerisi3 ? 1 : 0); // Status slot 3
  
  // Kirim data biaya (jika perlu)
  Blynk.virtualWrite(V6, biaya1);
  Blynk.virtualWrite(V7, biaya2);
  Blynk.virtualWrite(V8, biaya3);
  
  // Kirim data jarak sensor
  Blynk.virtualWrite(V9, bacaJarak(PIN_TRIG1, PIN_ECHO1));
  Blynk.virtualWrite(V10, bacaJarak(PIN_TRIG2, PIN_ECHO2));
  Blynk.virtualWrite(V11, bacaJarak(PIN_TRIG3, PIN_ECHO3));
}

// ==========================================================
// == FUNGSI TAMPILAN LCD (OPTIMIZED) ==
// ==========================================================

/**
 * Gambar tampilan awal sekali saja
 */
void gambarTampilanAwal(Adafruit_ILI9341 &lcd, const char* judul) {
  lcd.fillScreen(ILI9341_BLACK);
  
  // Header
  lcd.fillRect(0, 0, 320, 40, ILI9341_DARKCYAN);
  lcd.setCursor(20, 10);
  lcd.setTextColor(ILI9341_WHITE);
  lcd.setTextSize(3);
  lcd.print(judul);
  
  // Status Blynk indicator kecil
  lcd.fillRect(280, 5, 30, 15, ILI9341_BLACK);
  lcd.setCursor(282, 7);
  lcd.setTextColor(statusKoneksiBlynk ? ILI9341_GREEN : ILI9341_RED);
  lcd.setTextSize(1);
  lcd.print(statusKoneksiBlynk ? "ON" : "OFF");
  
  // Status Box (default KOSONG)
  lcd.fillRect(40, 60, 240, 70, ILI9341_GREEN);
  lcd.setCursor(90, 80);
  lcd.setTextColor(ILI9341_BLACK);
  lcd.setTextSize(4);
  lcd.print("KOSONG");
  
  // Footer info
  lcd.drawFastHLine(0, 150, 320, ILI9341_DARKGREY);
  
  lcd.setTextSize(2);
  lcd.setCursor(20, 170);
  lcd.setTextColor(ILI9341_LIGHTGREY);
  lcd.print("DURASI   : ");
  
  lcd.setCursor(20, 200);
  lcd.print("BIAYA    : Rp ");
}

/**
 * Update hanya bagian yang berubah
 */
void perbaruiTampilanSlot(Adafruit_ILI9341 &lcd, int nomorSlot, bool terisi, unsigned long durasiMs) {
  String statusSekarang = terisi ? "TERISI" : "KOSONG";
  String* statusSebelumnya;
  
  // Tentukan status sebelumnya berdasarkan slot
  switch(nomorSlot) {
    case 1: statusSebelumnya = &statusSebelumnya1; break;
    case 2: statusSebelumnya = &statusSebelumnya2; break;
    case 3: statusSebelumnya = &statusSebelumnya3; break;
    default: return;
  }
  
  // Update status box hanya jika berubah
  if (statusSekarang != *statusSebelumnya) {
    *statusSebelumnya = statusSekarang;
    
    if (terisi) {
      lcd.fillRect(40, 60, 240, 70, ILI9341_RED);
      lcd.setCursor(100, 80);
      lcd.setTextColor(ILI9341_WHITE);
      lcd.setTextSize(4);
      lcd.print("TERISI");
    } else {
      lcd.fillRect(40, 60, 240, 70, ILI9341_GREEN);
      lcd.setCursor(90, 80);
      lcd.setTextColor(ILI9341_BLACK);
      lcd.setTextSize(4);
      lcd.print("KOSONG");
    }
  }
  
  // Update Blynk status indicator
  lcd.fillRect(280, 5, 30, 15, ILI9341_BLACK);
  lcd.setCursor(282, 7);
  lcd.setTextColor(statusKoneksiBlynk ? ILI9341_GREEN : ILI9341_RED);
  lcd.setTextSize(1);
  lcd.print(statusKoneksiBlynk ? "ON" : "OFF");
  
  // Update durasi (selalu update karena berubah terus)
  char teksDurasi[10];
  formatDurasi(durasiMs, teksDurasi);
  
  lcd.setTextSize(2);
  lcd.setCursor(150, 170);
  lcd.setTextColor(ILI9341_YELLOW);
  lcd.fillRect(150, 170, 150, 20, ILI9341_BLACK); // Hapus area sebelumnya
  lcd.print(teksDurasi);
  
  // Update biaya
  int biaya = hitungBiaya(durasiMs);
  lcd.setCursor(150, 200);
  lcd.setTextColor(ILI9341_CYAN);
  lcd.fillRect(150, 200, 100, 20, ILI9341_BLACK); // Hapus area sebelumnya
  lcd.print(biaya);
}

/**
 * Menghitung Biaya berdasarkan Durasi
 */
int hitungBiaya(unsigned long durasiMs) {
  if (durasiMs == 0) {
    return 0;
  }
  
  // Konversi durasi nyata ke jam virtual
  unsigned long jamVirtual = (durasiMs / SATU_JAM_VIRTUAL_MS);
  
  // Hitung blok 5 jam
  int blokLimaJam = jamVirtual / 5;
  
  return (blokLimaJam + 1) * 10000;
}

/**
 * Mengubah milidetik ke format HH:MM:SS
 */
void formatDurasi(unsigned long durasiMs, char* buffer) {
  if (durasiMs == 0) {
    sprintf(buffer, "00:00:00");
    return;
  }

  // Konversi ke detik virtual
  unsigned long totalDetikVirtual = durasiMs / (SATU_JAM_VIRTUAL_MS / 3600);
  
  int jam = totalDetikVirtual / 3600;
  int menit = (totalDetikVirtual % 3600) / 60;
  int detik = totalDetikVirtual % 60;

  sprintf(buffer, "%02d:%02d:%02d", jam, menit, detik);
}

/**
 * Mengirim data 1 byte ke 74HC595
 */
void kirimKeShiftRegister(byte data) {
  digitalWrite(PIN_LATCH, LOW);   // Siapkan chip
  SPI.transfer(data);             // Kirim data
  digitalWrite(PIN_LATCH, HIGH);  // Tampilkan data ke output
}

/**
 * Membaca jarak dari HC-SR04
 */
long bacaJarak(int pinTrig, int pinEcho) {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);
  
  long durasi = pulseIn(pinEcho, HIGH, 30000); // 30ms timeout
  
  return durasi * 0.0343 / 2;
}