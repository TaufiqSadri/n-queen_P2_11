# Solver N-Queens — Tapip Anonim (P2-11)

## Normal v1

[Normal/normal-v1.cpp](Normal/normal-v1.cpp) mengimplementasikan `std::vector<std::vector<int>> solveNQueens(int n)` dalam C++17. Fungsi menerima ukuran papan `4 ≤ n ≤ 12` dan mengembalikan semua susunan valid, masing-masing satu kali. Dalam setiap solusi, `solusi[baris]` berisi kolom ratu dengan indeks mulai dari 0.

### Algoritma

1. Backtracking menempatkan satu ratu per baris. Tiga bitmask menandai kolom dan dua arah diagonal yang terlarang pada baris saat ini.
2. Operasi bit menyisakan kolom aman. Untuk setiap pilihan, solver menandai kolom itu dan menggeser mask diagonal satu posisi untuk menghitung serangan pada baris berikutnya.
3. Pada baris pertama, solver mencari dari separuh kiri papan. Setiap susunan yang ditemukan menghasilkan satu susunan cermin melalui `n - 1 - kolom`. Pada papan berukuran ganjil, cabang kolom tengah ditelusuri lengkap dan setiap hasilnya disimpan sekali.

Setiap jalur menempatkan ratu hanya pada kolom yang aman, dan satu susunan menentukan satu urutan pilihan. Solusi dari separuh kiri dan hasil cerminnya memiliki posisi ratu pertama di sisi yang berbeda. Cabang kolom tengah terpisah dari keduanya. Karena itu, pencarian menghasilkan semua solusi tanpa duplikasi.

### Waktu dan memori

Pemeriksaan konflik dan pengambilan satu kandidat dari bitmask berbiaya konstan. Jika `T` adalah jumlah keadaan pencarian yang dikunjungi dan `S` jumlah solusi, waktu total adalah `O(T + n × S)`, termasuk menyalin posisi ratu ke hasil. Batas kasar pencarian adalah `O(n!)` karena kolom tidak dapat dipakai ulang, dan konflik diagonal memangkas cabang lebih awal. Solver brute force bawaan menghasilkan setiap permutasi kolom lengkap sebelum memeriksa diagonal, dengan batas waktu `O(n! × n²)`.

Ruang tambahan pencarian berorde `O(n)` untuk jalur dan rekursi. Jalur menggunakan array lokal dengan kapasitas 12, dan pencarian tidak memerlukan buffer heap terpisah. Heap berorde `O(n × S)` untuk hasil. `reserve()` pada vector luar memakai jumlah solusi yang dicantumkan di README tester untuk menghindari pertumbuhan kapasitas berulang. Isi solusi tetap dibuat melalui pencarian.

## Normal v2

[Normal/normal-v2.cpp](Normal/normal-v2.cpp) memperluas pemangkasan simetri ke rotasi dan pencerminan papan.

V2 memakai backtracking dengan tiga bitmask dan memanfaatkan empat rotasi papan beserta pencerminannya. Pencarian mengutamakan orientasi yang menempatkan ratu baris pertama paling dekat ke sudut. Jika kolom ratu itu berjarak `d` dari sudut, ratu di tepi lain yang berjarak kurang dari `d` dapat dikesampingkan. Rotasi atau pencerminan akan memindahkan ratu tersebut ke baris pertama dan menghasilkan daftar kolom yang lebih kecil. Jika ratu pertama berada di sudut, pencerminan diagonal juga memungkinkan pemangkasan berdasarkan posisi ratu baris kedua. Pada papan lengkap, solver memilih daftar kolom terkecil di antara orientasi setara dan mengeluarkan setiap rotasi dan pencerminan yang berbeda. Dasar bitmask mengikuti [Martin Richards, *Backtracking Algorithms in MCPL*](https://www.cl.cam.ac.uk/~mr10/backtrk.pdf). Pemilihan wakil grup simetri mengikuti gagasan [Matthias Engelhardt, *Using Symmetry for the Regular N Queens Problem*](https://www.nqueens.de/sub/SearchAlgoUseSymm.en.html).

V2 memeriksa kandidat baris berikutnya sebelum memanggil rekursi dan memproses dua tingkat pencarian per panggilan. Dengan `T` kandidat yang dicoba dan `S` solusi yang dikembalikan, waktu tetap berorde `O(T + n × S)`. Pemangkasan simetri mengurangi `T`. Matriks papan lengkap tidak disimpan selama pencarian. Ruang tambahannya `O(n)`, termasuk jalur, mask, dan rotasi sementara. Heap hasil tetap berorde `O(n × S)`.

## Normal v3

[Normal/normal-v3.cpp](Normal/normal-v3.cpp) mempertahankan pencarian bitmask dan simetri v2. Saat tinggal dua baris, solver menentukan penempatan langsung. Dua kolom tersisa yang bersebelahan langsung menggugurkan cabang karena ratu pada kedua baris tersebut pasti bertemu di diagonal.

Untuk membandingkan satu solusi dengan rotasi dan pencerminannya, solver mengemas setiap posisi kolom dalam 4 bit. Seluruh papan muat dalam satu bilangan 64 bit, sehingga orientasi bisa dibandingkan langsung. Solver menyimpan jalur pencarian dan mask per baris dalam tipe yang lebih kecil, lalu mengubah papan ke `std::vector<int>` saat mengembalikan hasil.

Kompleksitasnya tetap `O(T + n × S)` untuk waktu, dengan `T` kandidat yang dicoba dan `S` solusi. Ruang tambahan pencarian `O(n)`, dan heap hasil `O(n × S)`.

## Pre-placed v1

[Preplaced/preplaced-v1.cpp](Preplaced/preplaced-v1.cpp) mengimplementasikan `solvePreplaced(n, preplaced)` untuk papan berukuran `4 ≤ n ≤ 12`. Setiap solusi mencantumkan kolom ratu di seluruh baris, termasuk ratu terkunci. Jika papan tidak bisa diselesaikan, fungsi mengembalikan daftar kosong.

### Algoritma

1. Masukkan posisi ratu terkunci dan periksa konflik antarposisi. Untuk tiap baris kosong, hitung bitmask kolom yang masih aman dari semua ratu terkunci.
2. Jalankan backtracking hanya pada baris kosong. Setelah menempatkan ratu, geser bitmask serangan diagonal sesuai jarak ke baris kosong berikutnya. Jika baris itu kehabisan kandidat, hentikan cabang.
3. Simpan papan lengkap saat semua baris terisi. Jika `P = 0`, cari satu sisi papan dan tambahkan hasil cerminnya. Pada papan ganjil dengan ratu pertama di tengah, posisi ratu kedua menentukan sisi yang dicari.

Dasar pencarian bitmask untuk kasus ini dijelaskan dalam [Gent, Jefferson, dan Nightingale, *Complexity of n-Queens Completion*](https://eprints.whiterose.ac.uk/id/eprint/141513/1/n_queens_jair.pdf).

### Waktu dan memori

Jika `P` adalah jumlah ratu terkunci, `T` jumlah kandidat yang dicoba selama pencarian, dan `S` jumlah solusi, waktu total adalah `O(P + n + T + n × S)`. Kedalaman rekursi paling banyak `n - P`. Ruang tambahan untuk pencarian adalah `O(n)`, dengan heap `O(n × S)` untuk hasil.

## Pre-placed v2

[Preplaced/preplaced-v2.cpp](Preplaced/preplaced-v2.cpp) mempertahankan backtracking bitmask v1 dan menyimpan jadwal baris kosong dalam 4 byte per baris. Mask kolom memerlukan paling banyak 12 bit, sedangkan indeks dan jarak antarbaris masing-masing disimpan dalam 1 byte.

Jika sejak awal hanya ada satu atau dua baris kosong, solver menyelesaikannya langsung. Untuk dua baris, kedua penempatan akan berkonflik secara diagonal jika jarak dua kolom yang tersisa sama dengan jarak kedua baris. Jika ada setidaknya lima baris kosong dan papan berisi ratu terkunci, solver membandingkan hasil kali jumlah kandidat pada dua baris kosong pertama dari tiap ujung papan. Pencarian dimulai dari ujung dengan perkiraan cabang lebih sedikit. Jika perkiraannya sama, pencarian dimulai dari atas. Batas kompleksitasnya tetap sama dengan v1.

## Menjalankan dan mengunggah solver

Jalankan dari root repo pada macOS dengan Emscripten yang sudah terpasang.

```bash
source ~/emsdk/emsdk_env.sh
npm run build:solvers -- --native
npm run dev
```

Di tester, pilih kasus **Normal** dan solver **normal-v1**, **normal-v2**, atau **normal-v3** di grup **Submissions/Normal**. Untuk kasus **Pre-placed**, pilih **preplaced-v1** atau **preplaced-v2** di grup **Submissions/Preplaced**. Untuk kasus **Walled**, pilih **walled-v1**, **walled-v2**, **walled-v3**, atau **walled-v4** di grup **Submissions/Walled**. Klik **Apply changes** setelah mengubah papan, lalu klik **Run**.

Untuk leaderboard, pilih kategori yang sesuai dengan solver, lalu unggah satu file `.cpp`.

## Walled v1

`Walled/walled-v1.cpp` mengimplementasikan `solveWalled(n, grid)` dalam C++17. Hasilnya adalah semua papan dengan jumlah ratu maksimum, masing-masing sekali dan diurutkan row-major. Papan tanpa sel bebas menghasilkan satu papan kosong.

### Algoritma

1. Setiap garis dibagi menjadi segmen baris, kolom, dan dua diagonal. Hanya sel dinding yang tepat pada garis yang memutus segmen. Setiap segmen dan himpunan kandidat disimpan dalam tiga word 64-bit, cukup untuk 144 sel.
2. Pilih segmen baris dengan kandidat tersedikit. Coba satu ratu pada setiap kandidat, kemudian cabang tanpa ratu pada segmen tersebut. Setiap pilihan menghapus seluruh sel yang diserang dari kandidat.
3. Batas atas tambahan ratu adalah jumlah segmen yang masih memiliki kandidat, diambil minimum dari empat arah. Cabang dipangkas hanya jika jumlah ratu ditambah batas ini lebih kecil daripada rekor. Hasil lama dihapus ketika rekor membaik.
4. Untuk N >= 4 tanpa dinding, gunakan pencarian bitmask satu ratu per baris. N kecil tetap memakai pencarian umum agar maksimum parsial untuk N=2 dan N=3 tetap benar.

Pilihan dalam satu segmen saling terpisah: sebuah papan memiliki tepat satu posisi ratu atau tidak memiliki ratu pada segmen yang dipilih. Pemilihan segmen deterministik dan semua pilihan dicoba. Karena itu setiap papan optimal dicapai sekali. Batas segmen tidak pernah meremehkan kapasitas yang tersisa, sehingga pemangkasan tidak menghilangkan hasil optimal. Jalur disimpan pada array tetap; alokasi hanya dilakukan ketika menyimpan hasil atau snapshot debug. Template debug menghilangkan penghitung dan snapshot dari jalur normal.

### Waktu dan memori

Untuk F sel bebas, batas kasar pencarian adalah eksponensial O(2^F); masalah umum ini tidak memiliki jaminan satu detik untuk setiap susunan dinding. Tiap simpul pencarian menghitung batas dengan O(F) operasi word karena kapasitas bitset tetap. Penyalinan dan pengurutan S hasil berukuran K membutuhkan O(S K log K). Memori kerja tetap untuk maksimum 144 sel, dengan kedalaman rekursi paling besar F; hasil membutuhkan O(S K). Jumlah hasil yang sangat besar juga dapat melampaui batas memori 512 MB, sehingga benchmark terbatas tidak membuktikan batas universal.

### Pengujian dan pengiriman

```bash
source ~/emsdk/emsdk_env.sh
npm install
npm run typecheck
npm run build:solvers -- --native
```

Pilih **Submissions/Walled → walled-v1** dan kasus **Walled**. Gunakan versi ini sebagai pembanding untuk v2 dan v3.

### Hasil validasi lokal

Pengujian native: 2.100 papan acak N=1..7 dibandingkan sebagai himpunan lengkap dengan `Solver/example/bruteforce-walled.cpp`, termasuk tanpa dinding dan semua sel berdinding. Semua cocok tanpa duplikasi. Tambahan 150 papan dengan banyak dinding pada N=8..12 (30 seed per ukuran) lolos validator independen; 30 papan N=8 di antaranya juga cocok persis dengan brute force. Jumlah tanpa dinding untuk N=4..12 cocok dengan 2, 10, 4, 40, 92, 352, 724, 2680, 14200. Validator independen memeriksa setiap pasangan ratu dengan berjalan sel demi sel sampai dinding pada seluruh hasil benchmark.

Mesin: Linux x86_64, Intel Core i5-11320H @ 3.20 GHz, GCC 16.1.1 20260515 (Red Hat 16.1.1-2), `g++ -std=c++17 -O2`. Tabel adalah waktu minimum tujuh panggilan solver, termasuk pembuatan hasil tetapi tidak termasuk validasi. Generator memakai `std::mt19937(seed)` dan `std::shuffle` atas indeks sel, lalu memilih W indeks pertama sebagai dinding. Tidak ada kasus tabel yang melebihi 1.000 ms. Angka ini native, bukan WASM atau leaderboard.

## Walled v2

[Walled/walled-v2.cpp](Walled/walled-v2.cpp) memakai bitmask per baris untuk papan dengan sedikit segmen tambahan. Dinding menghentikan rambatan serangan kolom atau diagonal sebelum mask digeser ke baris berikutnya. Pada papan kosong, solver memakai simetri cermin. Pada papan dengan lebih banyak segmen, solver mengelompokkan kandidat yang saling menyerang dan memakai jumlah kelompok sebagai batas atas.

Dalam pengujian WASM lokal, v2 mengembalikan himpunan solusi yang sama dengan v1 pada 75 papan berukuran 8 sampai 12 dan lebih cepat pada 69 papan tersebut. Waktu terbaik dari tujuh percobaan pada papan 12×12 dengan satu dinding (seed 33) turun dari 570,1 ms ke 46,8 ms. Pengukuran memakai Intel Core i5-11320H, Emscripten 7.0.1, dan Node.js 22.22.2.

## Walled v3

[Walled/walled-v3.cpp](Walled/walled-v3.cpp) membagi sel bebas menjadi segmen pada empat arah. Pencarian berjalan pada arah baris atau kolom yang memiliki lebih sedikit segmen. Setiap segmen menyimpan kandidat dalam bitmask 16-bit, cukup untuk satu ruas dengan paling banyak 12 sel. Solver memilih segmen dengan kandidat tersedikit, lalu mencoba setiap posisi ratu yang masih aman dan pilihan melewati segmen jika jumlah target masih dapat tercapai.

Pencarian dimulai dari target sebesar jumlah segmen pada arah yang dipilih. Jika target belum tercapai, solver menurunkannya satu per satu. Pada target pertama yang tercapai, solver menyelesaikan pencarian untuk mengembalikan semua susunan optimal dalam urutan koordinat baris lalu kolom. Papan penuh dinding menghasilkan satu susunan kosong.

Hasil native cocok dengan brute force pada seluruh 66.066 tata letak dinding untuk ukuran 1 sampai 4. Build WebAssembly berhasil, dan papan kosong 12×12 menghasilkan 14.200 solusi.

Untuk menguji v3, pilih kasus **Walled** dan solver **Submissions/Walled → walled-v3**. Untuk leaderboard, pilih kasus **Walled** dan unggah [Walled/walled-v3.cpp](Walled/walled-v3.cpp).

## Walled v4

V4 mengimplementasikan `solveWalled(n, grid)` dan mengembalikan semua susunan dengan jumlah ratu maksimum. Koordinat di setiap susunan diurutkan berdasarkan baris lalu kolom. File solver tersedia di [Walled/walled-v4.cpp](Walled/walled-v4.cpp).

### Algoritma

- Pada papan tanpa dinding, v4 memakai pencarian bitmask per baris dan menghasilkan pasangan solusi cermin. Pada papan dengan satu dinding, v4 memakai pencarian per baris dengan serangan yang berhenti di dinding.
- Pada papan dengan lebih banyak dinding, v4 mengemas setiap segmen ke dalam satu word 64-bit tanpa melintasi batas antarword. Mask serangan dibuat dengan menelusuri setiap arah hingga dinding atau tepi. Operasi bit menghitung segmen aktif dan memilih segmen dengan kandidat tersedikit.
- Matching maksimum antara segmen horizontal dan vertikal dihitung sekali sebagai batas atas awal. Solver menurunkan target hingga menemukan solusi dan mengumpulkan semua susunan pada target itu.

### Validasi dan benchmark lokal

V4 cocok dengan brute force pada 66.066 tata letak dinding untuk N=1 sampai 4 dan 2.100 papan acak untuk N=1 sampai 7. Hasilnya juga cocok dengan v3 pada 150 papan untuk N=8 sampai 12 dan enam kasus di sekitar batas 64 dan 128 sel bebas.

Tabel memakai harness WASM lokal pada Intel Core i5-11320H, Emscripten 7.0.1, dan Node.js 22.22.2. Waktu adalah nilai terbaik dari tujuh run setelah pemanasan. Memori dihitung oleh tester dalam byte. Papan acak memakai seed 22, kecuali papan satu dinding yang memakai seed 33.

| Papan 12×12 | Solusi | v2 (ms) | v3 (ms) | v4 (ms) | Memori v3 (B) | Memori v4 (B) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Tanpa dinding | 14.200 | 4,655 | 15,116 | 4,522 | 1.650.231 | 1.600.844 |
| 1 dinding | 71.710 | 14,547 | 25,714 | 15,341 | 8.283.307 | 8.234.624 |
| 6 dinding | 494 | 15,561 | 2,830 | 1,626 | 119.893 | 78.071 |
| 24 dinding | 3 | 2,318 | 2,150 | 0,889 | 52.861 | 8.523 |
| 80 dinding | 2.088 | 19,416 | 58,677 | 21,453 | 520.625 | 473.556 |
| 9 segmen terpisah | 19.683 | 3,036 | 2,773 | 2,161 | 1.799.958 | 1.755.683 |

### Build dan submit

Jalankan dari root repo dalam terminal yang sama.

```bash
source ~/emsdk/emsdk_env.sh
npm run build:solvers -- --native
```

Cek baris `Building Submissions/Walled/walled-v4 ... ok`, lalu pilih kasus **Walled** dan solver **Submissions/Walled → walled-v4**.

Untuk leaderboard, pilih kasus **Walled** dan unggah langsung file sumber [Walled/walled-v4.cpp](Walled/walled-v4.cpp). Jika muncul `Execution Error (missing)`, periksa kategori dan nama file pada detail submission. Kategori menentukan fungsi yang dipanggil, dan v4 menyediakan `solveWalled()`.
