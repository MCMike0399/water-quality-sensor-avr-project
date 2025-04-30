// Configuración para almacenar historial de datos
const MAX_DATA_POINTS = 50;
const chartData = {
    time: [],
    turbidity: [],
    ph: [],
    conductivity: []
};

// Umbrales actualizados para alertas y estados basados en valores reales de sensores
const THRESHOLDS = {
    PH: {
        ideal: { min: 6.5, max: 7.5 },
        good: { min: 6.0, max: 8.0 },
        acceptable: { min: 5.0, max: 9.0 },
        warning: { min: 3, max: 11 },
        danger: { min: 2, max: 12 }
    },
    T: {
        ideal: { max: 10 },
        good: { max: 50 },
        acceptable: { max: 100 },
        warning: { max: 500 },
        danger: { max: 800 }
    },
    C: {
        ideal: { max: 300 },
        good: { max: 600 },
        acceptable: { max: 900 },
        warning: { max: 1200 },
        danger: { max: 1400 }
    }
};

// Inicializar gráficos separados
function initCharts() {
    // Gráfico de Turbidez
    const turbidityTrace = {
        x: chartData.time,
        y: chartData.turbidity,
        name: 'Turbidez',
        type: 'scatter',
        line: {color: '#3498db', width: 2}
    };

    const turbidityLayout = {
        title: 'Turbidez en Tiempo Real',
        margin: { l: 50, r: 20, t: 50, b: 80 },
        xaxis: {
            title: { 
                text: 'Tiempo',
            },
            showgrid: true
        },
        yaxis: {
            title: 'Turbidez (NTU)',
            titlefont: {color: '#3498db'},
            tickfont: {color: '#3498db'},
            range: [0, 1000] // Rango actualizado para turbidez (0-1000 NTU)
        }
    };

    Plotly.newPlot('turbidityChart', [turbidityTrace], turbidityLayout, {responsive: true});

    // Gráfico de pH
    const phTrace = {
        x: chartData.time,
        y: chartData.ph,
        name: 'pH',
        type: 'scatter',
        line: {color: '#e74c3c', width: 2}
    };

    const phLayout = {
        title: 'pH en Tiempo Real',
        margin: { l: 50, r: 20, t: 50, b: 80 },
        xaxis: {
            title: { 
                text: 'Tiempo',
            },
            showgrid: true
        },
        yaxis: {
            title: 'pH',
            titlefont: {color: '#e74c3c'},
            tickfont: {color: '#e74c3c'},
            range: [0, 14] // Rango exacto para pH (0-14)
        }
    };

    Plotly.newPlot('phChart', [phTrace], phLayout, {responsive: true});

    // Gráfico de Conductividad
    const conductivityTrace = {
        x: chartData.time,
        y: chartData.conductivity,
        name: 'Conductividad',
        type: 'scatter',
        line: {color: '#2ecc71', width: 2}
    };

    const conductivityLayout = {
        title: 'Conductividad en Tiempo Real',
        margin: { l: 60, r: 20, t: 50, b: 80 },
        xaxis: {
            title: { 
                text: 'Tiempo',
                standoff: 20 // Espacio adicional para el título
            },
            showgrid: true
        },
        yaxis: {
            title: 'Conductividad (μS/cm)',
            titlefont: {color: '#2ecc71'},
            tickfont: {color: '#2ecc71'},
            range: [0, 1500] // Rango actualizado para conductividad (0-1500 μS/cm)
        }
    };

    Plotly.newPlot('conductivityChart', [conductivityTrace], conductivityLayout, {responsive: true});
}

// Función para actualizar gráficos con nuevos datos
function updateCharts(data) {
    const now = new Date();
    const timeStr = now.toLocaleTimeString();
    
    // Añadir nuevo punto de datos
    chartData.time.push(timeStr);
    chartData.turbidity.push(data.T);
    chartData.ph.push(data.PH);
    chartData.conductivity.push(data.C);
    
    // Limitar el número de puntos
    if (chartData.time.length > MAX_DATA_POINTS) {
        chartData.time.shift();
        chartData.turbidity.shift();
        chartData.ph.shift();
        chartData.conductivity.shift();
    }
    
    // Actualizar cada gráfico individualmente
    Plotly.update('turbidityChart', {
        x: [chartData.time],
        y: [chartData.turbidity]
    }, {}, [0]);
    
    Plotly.update('phChart', {
        x: [chartData.time],
        y: [chartData.ph]
    }, {}, [0]);
    
    Plotly.update('conductivityChart', {
        x: [chartData.time],
        y: [chartData.conductivity]
    }, {}, [0]);
}

// Formatear valores para mostrar (sin conversión)
function formatValues(data) {
    return {
        T: parseFloat(data.T).toFixed(2),
        PH: parseFloat(data.PH).toFixed(2),
        C: parseFloat(data.C).toFixed(0)
    };
}

// Evaluar el estado del pH y devolver mensaje y clase CSS
function evaluatePh(ph) {
    const value = parseFloat(ph);
    
    if (value >= THRESHOLDS.PH.ideal.min && value <= THRESHOLDS.PH.ideal.max) {
        return {
            status: "Ideal para la mayoría de organismos acuáticos",
            class: "alert-success"
        };
    } else if (value >= THRESHOLDS.PH.good.min && value <= THRESHOLDS.PH.good.max) {
        return {
            status: "Buen estado - Rango aceptable",
            class: "alert-success"
        };
    } else if (value >= THRESHOLDS.PH.acceptable.min && value <= THRESHOLDS.PH.acceptable.max) {
        return {
            status: "Aceptable - Monitorear",
            class: "alert-info"
        };
    } else if (value < THRESHOLDS.PH.warning.min && value >= THRESHOLDS.PH.danger.min) {
        return {
            status: "Advertencia: pH muy ácido",
            class: "alert-warning"
        };
    } else if (value > THRESHOLDS.PH.warning.max && value <= THRESHOLDS.PH.danger.max) {
        return {
            status: "Advertencia: pH muy alcalino",
            class: "alert-warning"
        };
    } else if (value < THRESHOLDS.PH.danger.min) {
        return {
            status: "PELIGRO: pH extremadamente ácido",
            class: "alert-danger"
        };
    } else if (value > THRESHOLDS.PH.danger.max) {
        return {
            status: "PELIGRO: pH extremadamente alcalino",
            class: "alert-danger"
        };
    } else {
        return {
            status: "Estado indeterminado",
            class: "alert-info"
        };
    }
}

// Evaluar el estado de la turbidez
function evaluateTurbidity(turbidity) {
    const value = parseFloat(turbidity);
    
    if (value <= THRESHOLDS.T.ideal.max) {
        return {
            status: "Excelente claridad del agua",
            class: "alert-success"
        };
    } else if (value <= THRESHOLDS.T.good.max) {
        return {
            status: "Buena claridad - Rango aceptable",
            class: "alert-success"
        };
    } else if (value <= THRESHOLDS.T.acceptable.max) {
        return {
            status: "Agua ligeramente turbia - Aceptable",
            class: "alert-info"
        };
    } else if (value <= THRESHOLDS.T.warning.max) {
        return {
            status: "Advertencia: Agua turbia",
            class: "alert-warning"
        };
    } else if (value > THRESHOLDS.T.danger.max) {
        return {
            status: "PELIGRO: Turbidez muy elevada",
            class: "alert-danger"
        };
    } else {
        return {
            status: "Estado indeterminado",
            class: "alert-info"
        };
    }
}

// Evaluar el estado de la conductividad
function evaluateConductivity(conductivity) {
    const value = parseFloat(conductivity);
    
    if (value <= THRESHOLDS.C.ideal.max) {
        return {
            status: "Excelente - Agua muy pura",
            class: "alert-success"
        };
    } else if (value <= THRESHOLDS.C.good.max) {
        return {
            status: "Buena calidad - Rango normal",
            class: "alert-success"
        };
    } else if (value <= THRESHOLDS.C.acceptable.max) {
        return {
            status: "Aceptable - Monitorear",
            class: "alert-info"
        };
    } else if (value <= THRESHOLDS.C.warning.max) {
        return {
            status: "Advertencia: Conductividad elevada",
            class: "alert-warning"
        };
    } else if (value > THRESHOLDS.C.danger.max) {
        return {
            status: "PELIGRO: Conductividad muy alta",
            class: "alert-danger"
        };
    } else {
        return {
            status: "Estado indeterminado",
            class: "alert-info"
        };
    }
}

// Comprobar valores contra umbrales y actualizar alertas principales
function checkThresholds(data) {
    const alertPanel = document.getElementById('alertPanel');
    const alertMessage = document.getElementById('alertMessage');
    
    // Convertir strings a números
    const ph = parseFloat(data.PH);
    const turbidity = parseFloat(data.T);
    const conductivity = parseFloat(data.C);
    
    // Actualizar indicadores de estado individuales
    const phEvaluation = evaluatePh(ph);
    const turbidityEvaluation = evaluateTurbidity(turbidity);
    const conductivityEvaluation = evaluateConductivity(conductivity);
    
    // Actualizar clase y mensaje de estado para cada sensor
    const phStatus = document.getElementById('phStatus');
    phStatus.textContent = phEvaluation.status;
    phStatus.className = `sensor-status ${phEvaluation.class}`;
    
    const turbidityStatus = document.getElementById('turbidityStatus');
    turbidityStatus.textContent = turbidityEvaluation.status;
    turbidityStatus.className = `sensor-status ${turbidityEvaluation.class}`;
    
    const conductivityStatus = document.getElementById('conductivityStatus');
    conductivityStatus.textContent = conductivityEvaluation.status;
    conductivityStatus.className = `sensor-status ${conductivityEvaluation.class}`;
    
    
    // Si todos los valores están en rangos aceptables
    if (ph >= THRESHOLDS.PH.acceptable.min && ph <= THRESHOLDS.PH.acceptable.max &&
        turbidity <= THRESHOLDS.T.acceptable.max &&
        conductivity <= THRESHOLDS.C.acceptable.max) {
        
        // Si alguno está en rango ideal, mostrar mensaje positivo
        if ((ph >= THRESHOLDS.PH.ideal.min && ph <= THRESHOLDS.PH.ideal.max) ||
            turbidity <= THRESHOLDS.T.ideal.max ||
            conductivity <= THRESHOLDS.C.ideal.max) {
            
            alertPanel.style.display = 'block';
            alertPanel.className = 'alert alert-success';
            alertMessage.textContent = 'Todos los parámetros se encuentran en rangos aceptables o ideales.';
            return;
        }
    }
    
    // Si llegamos aquí, no hay alertas principales activas
    alertPanel.style.display = 'none';
}

// Actualizar interfaz con nuevos valores
function updateInterface(data) {
    // Solo formatear los valores para mostrar (no necesita conversión)
    const formattedData = formatValues(data);
    
    // Actualizar indicadores
    document.getElementById('turbidity').textContent = formattedData.T;
    document.getElementById('ph').textContent = formattedData.PH;
    document.getElementById('conductivity').textContent = formattedData.C;
    
    // Comprobar valores contra umbrales
    checkThresholds(formattedData);
    
    // Actualizar timestamp
    const now = new Date();
    document.getElementById('lastUpdate').textContent = 
        `Última actualización: ${now.toLocaleTimeString()}`;
    
    // Actualizar gráficos
    updateCharts(data);
}

// Conectar WebSocket
function connectWebSocket() {
    const statusElement = document.getElementById('connection');
    
    statusElement.textContent = 'Conectando...';
    
    const ws = new WebSocket('ws://' + window.location.host + '/ws');
    
    ws.onopen = function() {
        statusElement.textContent = 'Conectado';
        statusElement.className = 'status connected';
        console.log('WebSocket conectado');
    };
    
    ws.onmessage = function(event) {
        const data = JSON.parse(event.data);
        console.log('Datos recibidos:', data);
        updateInterface(data);
    };
    
    ws.onclose = function() {
        statusElement.textContent = 'Desconectado - Reconectando...';
        statusElement.className = 'status disconnected';
        // Reconectar después de 2 segundos
        setTimeout(connectWebSocket, 2000);
    };
    
    ws.onerror = function(err) {
        console.error('Error en WebSocket:', err);
        ws.close();
    };
}

// Inicializar cuando la página cargue
window.addEventListener('load', function() {
    // Inicializar gráficos
    initCharts();
    
    // Conectar WebSocket
    connectWebSocket();

    // Manejar el redimensionamiento de la ventana
    window.addEventListener('resize', function() {
        Plotly.Plots.resize('turbidityChart');
        Plotly.Plots.resize('phChart');
        Plotly.Plots.resize('conductivityChart');
    });
});