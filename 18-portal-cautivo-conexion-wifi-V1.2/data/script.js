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

document.addEventListener("DOMContentLoaded", function () {
    const form = document.querySelector("form");
    const mensaje = document.createElement("p"); // Mensaje de estado
    mensaje.style.color = "blue";
    mensaje.style.fontWeight = "bold";
    mensaje.textContent = "";
    form.appendChild(mensaje);

    form.addEventListener("submit", async function (event) {
        event.preventDefault(); // Evita que la página recargue

        const ssid = document.getElementById("SSID").value;
        const password = document.getElementById("PASSWORD").value;

        if (!ssid || !password) {
            mensaje.textContent = "Por favor, ingrese un SSID y una contraseña.";
            mensaje.style.color = "red";
            return;
        }

        mensaje.textContent = "Intentando conectar a la red Wi-Fi...";
        mensaje.style.color = "blue";

        try {
            let response = await fetch(`/submit?SSID=${encodeURIComponent(ssid)}&PASSWORD=${encodeURIComponent(password)}`);
            let text = await response.text();

            if (text.includes("Intentando conectar")) {
                mensaje.textContent = "Conectando... Esto puede tardar unos segundos.";
                esperarConexion(mensaje);  // **Iniciar el proceso de espera**
            } else {
                mensaje.textContent = "Error al conectar. Intente nuevamente.";
                mensaje.style.color = "red";
            }
        } catch (error) {
            mensaje.textContent = "Error en la conexión con el ESP32.";
            mensaje.style.color = "red";
        }
    });
});

// **Nueva función para esperar a que el ESP32 se conecte**
async function esperarConexion(mensaje) {
    let intentos = 0;
    const intervalo = setInterval(async () => {
        try {
            let response = await fetch("/status");
            let estado = await response.text();

            if (estado === "CONNECTED") {
                mensaje.textContent = "Conexión exitosa. Redirigiendo...";
                mensaje.style.color = "green";
                clearInterval(intervalo);

                setTimeout(() => {
                    window.location.href = "about:blank";  // Cierra la página después de 5 segundos
                }, 5000);
            } else {
                mensaje.textContent = `Intentando conectar... (${intentos + 1})`;
                intentos++;
            }

            if (intentos >= 10) {  // Si después de 10 intentos (20 segundos) no conecta, detener.
                mensaje.textContent = "No se pudo conectar a la red Wi-Fi.";
                mensaje.style.color = "red";
                clearInterval(intervalo);
            }
        } catch (error) {
            console.error("Error verificando estado de conexión", error);
        }
    }, 2000);
}



window.onload = cargarRedesWiFi;
