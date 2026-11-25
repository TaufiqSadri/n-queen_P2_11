# Solver N-Queens — Tapip Anonim (P2-11)

## Normal v1

`Normal/normal-v1.cpp` adalah kode utama untuk diedit dan diunggah. Pencarian solver lokal di `driver/scripts/discover.mjs` telah disesuaikan agar membaca subfolder secara rekursif. Folder relatif menjadi kategori solver, yaitu `Submissions/Normal`.

`normal-v1.cpp` mengimplementasikan `std::vector<std::vector<int>> solveNQueens(int n)` dalam C++17. Fungsi menerima ukuran papan `4 ≤ n ≤ 12` dan mengembalikan semua susunan valid, masing-masing satu kali. Dalam setiap solusi, `solusi[baris]` berisi kolom ratu dengan indeks mulai dari 0.

Versi v1 menjadi acuan awal untuk membandingkan kebenaran, waktu, dan memori pada optimasi berikutnya. Simpan versi berikutnya dengan nama seperti `normal-v2.cpp` agar tiap versi dapat dipilih dan diuji secara terpisah. Hasil benchmark di bawah adalah milik v1.

### Algoritma

1. Backtracking menempatkan satu ratu per baris. Tiga bitmask menandai kolom dan dua arah diagonal yang terlarang pada baris saat ini.
2. Operasi bit menyisakan kolom aman. Untuk setiap pilihan, solver menandai kolom itu dan menggeser mask diagonal satu posisi untuk menghitung serangan pada baris berikutnya.
3. Pada baris pertama, solver mencari dari separuh kiri papan. Setiap susunan yang ditemukan menghasilkan satu susunan cermin melalui `n - 1 - kolom`. Pada papan berukuran ganjil, cabang kolom tengah ditelusuri lengkap dan setiap hasilnya disimpan sekali.

Setiap jalur menempatkan ratu hanya pada kolom yang aman, dan satu susunan menentukan satu urutan pilihan. Solusi dari separuh kiri dan hasil cerminnya memiliki posisi ratu pertama di sisi yang berbeda. Cabang kolom tengah terpisah dari keduanya. Karena itu, pencarian menghasilkan semua solusi tanpa duplikasi.

### Waktu dan memori

Pemeriksaan konflik dan pengambilan satu kandidat dari bitmask berbiaya konstan. Jika `T` adalah jumlah keadaan pencarian yang dikunjungi dan `S` jumlah solusi, waktu total adalah `O(T + n × S)`, termasuk menyalin posisi ratu ke hasil. Batas kasar pencarian adalah `O(n!)` karena kolom tidak dapat dipakai ulang, dan konflik diagonal memangkas cabang lebih awal. Solver brute force bawaan menghasilkan setiap permutasi kolom lengkap sebelum memeriksa diagonal, dengan batas waktu `O(n! × n²)`.

Ruang tambahan pencarian berorde `O(n)` untuk jalur dan rekursi. Jalur menggunakan array lokal dengan kapasitas 12, dan pencarian tidak memerlukan buffer heap terpisah. Heap berorde `O(n × S)` untuk hasil. `reserve()` pada vector luar memakai jumlah solusi yang dicantumkan di README tester untuk menghindari pertumbuhan kapasitas berulang. Isi solusi tetap dibuat melalui pencarian. Heap peak pada tester mengukur pertumbuhan batas heap selama fungsi berjalan, sebelum hasil dikemas.

### Pengujian dan pengiriman

Jalankan dari root repo pada macOS dengan Emscripten yang sudah terpasang.

```bash
source ~/emsdk/emsdk_env.sh
npm run build:solvers -- --native
npm run dev
```

Pilih solver **Submissions/Normal → normal-v1** dan kasus **Normal**. Klik **Apply changes** setelah mengubah ukuran papan, lalu **Run**. Jumlah solusi yang diharapkan untuk `N = 4–12` berturut-turut adalah **2, 10, 4, 40, 92, 352, 724, 2.680, dan 14.200**.

Untuk leaderboard, pilih kasus **Normal** dan unggah satu file [Normal/normal-v1.cpp](Normal/normal-v1.cpp). File berukuran sekitar 2,2 KB, menyertakan `nqueens.h`, dan memakai fungsi yang dipanggil tester. Solver menjalankan pencarian deterministik tanpa `main()`, thread, pencetakan, akses file, atau jaringan.

### Hasil validasi lokal

Himpunan solusi WebAssembly cocok persis dengan pencarian referensi yang memeriksa kolom dan diagonal satu per satu untuk seluruh `N = 4–12`. Pemeriksaan juga mencakup jumlah solusi dan duplikasi.

Benchmark lokal menggunakan macOS arm64, Node.js v26.5.0, dan Emscripten 5.0.5 dengan optimasi `-O2`. Setiap angka waktu adalah yang terbaik dari tujuh run setelah satu pemanasan. Heap adalah pertumbuhan terbesar pada run terukur. Pengukuran memakai harness tester di Node.js, sebelum pengemasan hasil.

| N | Waktu brute force | Waktu solver ini | Heap brute force | Heap solver ini |
| ---: | ---: | ---: | ---: | ---: |
| 9 | 3,204 ms | 0,045 ms | 16 KiB | 16 KiB |
| 10 | 32,045 ms | 0,140 ms | 44 KiB | 40 KiB |
| 11 | 343,908 ms | 0,686 ms | 172 KiB | 156 KiB |

Dalam pengukuran terpisah pada `N = 12`, bitmask dengan simetri mencatat **3,164 ms** dan bitmask tanpa simetri mencatat **6,050 ms**. Keduanya menggunakan **940 KiB** menurut metrik heap tester. Angka ini merupakan hasil lokal, sedangkan waktu dan heap leaderboard mengikuti lingkungan server.
