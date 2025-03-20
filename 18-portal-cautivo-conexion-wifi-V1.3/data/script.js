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

/**
 * Envía las credenciales (SSID, PASSWORD) al servidor y
 * actualiza la interfaz según el resultado.
 */
async function submitForm(event) {
    event.preventDefault();  // Evitar el envío tradicional del formulario

    // Mostrar un mensaje de "Intentando..."
    const estadoConexion = document.getElementById("estadoConexion");
    estadoConexion.textContent = "Intentando establecer conexión...";

    // Tomar los valores del formulario
    const ssid = document.getElementById("SSID").value;
    const password = document.getElementById("PASSWORD").value;

    try {
        // Hacemos la solicitud a /submit con los datos
        const response = await fetch(`/submit?SSID=${encodeURIComponent(ssid)}&PASSWORD=${encodeURIComponent(password)}`);
        const text = await response.text(); // El servidor enviará algo como "OK" o "FAIL" o un texto de error

        // Si la respuesta contiene "OK", asumimos que fue exitosa
        if (text.includes("OK")) {
            estadoConexion.textContent = "Dispositivo conectado. espere a que el LED se establezca en color verde y cierre la pagina web";
            // Aquí podrías hacer otro fetch si deseas apagar el servidor, o simplemente dejar el mensaje.
        } else {
            // Si no incluye "OK", mostramos el texto como error
            estadoConexion.textContent = "Error al conectar: " + text;
        }
    } catch (err) {
        estadoConexion.textContent = "Error de red: " + err;
        console.error(err);
    }
}

// Cuando cargue la página:
window.onload = () => {
    // Escanear redes
    cargarRedesWiFi();
    // Interceptar envío de formulario
    const form = document.querySelector("form");
    form.addEventListener("submit", submitForm);
};
