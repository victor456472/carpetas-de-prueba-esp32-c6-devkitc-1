async function cargarRedesWiFi() {
    try {
        let response = await fetch("/scan");
        let redes = await response.json();
        let select = document.getElementById("SSID");

        select.innerHTML = "<option value=''>Seleccione una red...</option>"; // Limpiar opciones
        
        redes.forEach(ssid => {
            let option = document.createElement("option");
            option.value = ssid;
            option.textContent = ssid;
            select.appendChild(option);
        });

    } catch (error) {
        console.error("Error al obtener redes Wi-Fi", error);
    }
}

window.onload = cargarRedesWiFi;
