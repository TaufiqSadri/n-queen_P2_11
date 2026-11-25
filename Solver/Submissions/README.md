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

## Pre-placed v1

[Preplaced/preplaced-v1.cpp](Preplaced/preplaced-v1.cpp) mengimplementasikan `solvePreplaced(n, preplaced)` untuk papan berukuran `4 ≤ n ≤ 12`. Setiap solusi mencantumkan kolom ratu di seluruh baris, termasuk ratu terkunci. Jika papan tidak bisa diselesaikan, fungsi mengembalikan daftar kosong.

### Algoritma

1. Masukkan posisi ratu terkunci dan periksa konflik antarposisi. Untuk tiap baris kosong, hitung bitmask kolom yang masih aman dari semua ratu terkunci.
2. Jalankan backtracking hanya pada baris kosong. Setelah menempatkan ratu, geser bitmask serangan diagonal sesuai jarak ke baris kosong berikutnya. Jika baris itu kehabisan kandidat, hentikan cabang.
3. Simpan papan lengkap saat semua baris terisi. Jika `P = 0`, cari satu sisi papan dan tambahkan hasil cerminnya. Pada papan ganjil dengan ratu pertama di tengah, posisi ratu kedua menentukan sisi yang dicari.

Dasar pencarian bitmask untuk kasus ini dijelaskan dalam [Gent, Jefferson, dan Nightingale, *Complexity of n-Queens Completion*](https://eprints.whiterose.ac.uk/id/eprint/141513/1/n_queens_jair.pdf).

### Waktu dan memori

Jika `P` adalah jumlah ratu terkunci, `T` jumlah kandidat yang dicoba selama pencarian, dan `S` jumlah solusi, waktu total adalah `O(P + n + T + n × S)`. Kedalaman rekursi paling banyak `n - P`. Ruang tambahan untuk pencarian adalah `O(n)`, dengan heap `O(n × S)` untuk hasil.

## Menjalankan dan mengunggah solver

Jalankan dari root repo pada macOS dengan Emscripten yang sudah terpasang.

```bash
source ~/emsdk/emsdk_env.sh
npm run build:solvers -- --native
npm run dev
```

Di tester, pilih kasus **Normal** dan solver **normal-v1** atau **normal-v2** di grup **Submissions/Normal**. Untuk kasus **Pre-placed**, pilih **preplaced-v1** di grup **Submissions/Preplaced**. Klik **Apply changes** setelah mengubah papan, lalu klik **Run**.

Untuk leaderboard, pilih kategori yang sesuai dengan solver, lalu unggah satu file `.cpp`.
