// const url = 'http://192.168.0.6:5000/data'; // Mengatur alamat ip secara manual
const base = window.location.hostname;
const url = `http://${base}:5000/data`;
const originalTextContent = {};

function fetchData() {
    fetch(url)
        .then(response => response.json())
        .then(data => {
            const slots = document.querySelectorAll('.slot');

            if (data.status && data.status === "ESP32 Not Available") {
                // Jika ESP32 tidak tersedia, ubah warna seluruh slot menjadi putih dan nomor diganti dengan -
                slots.forEach(slot => {
                    slot.style.backgroundColor = 'white';
                    const textElement = slot.querySelector('.text');
                    if (textElement) {
                        textElement.textContent = '-';
                    }
                });
            } else {
                for (const slot in data) {
                    const slotStatus = data[slot];
                    const slotElement = document.querySelector(`.slot${slot}`);
                    const textElement = slotElement ? slotElement.querySelector('.text') : null;

                    if (!originalTextContent[slot] && textElement) {
                        originalTextContent[slot] = textElement.textContent;
                    }

                    if (slotElement) {
                        // Periksa status slot dan ubah warna serta teks
                        if (slotStatus === "1") {
                            slotElement.style.backgroundColor = 'green'; // Terisi, ubah warna menjadi hijau
                            if (textElement) {
                                textElement.textContent = originalTextContent[slot]; // Tampilkan nilai asli
                            }
                        } else if (slotStatus === "0") {
                            slotElement.style.backgroundColor = 'gray'; // Kosong, ubah warna menjadi abu-abu
                            if (textElement) {
                                textElement.textContent = originalTextContent[slot]; // Tampilkan nilai asli
                            }
                        }
                        // Jika statusnya 'N', ubah teks menjadi 'NA'
                        else if (slotStatus === "N") {
                            slotElement.style.backgroundColor = 'white'; // Ubah warna menjadi kuning (atau warna lain yang diinginkan)
                            if (textElement) {
                                textElement.textContent = 'NA';
                            }
                        }
                    }
                }
            }
        })
        .catch(error => console.error('Error fetching data:', error));
}

// Menjalankan fetchData secara berkala
setInterval(fetchData, 1000);
fetchData();
