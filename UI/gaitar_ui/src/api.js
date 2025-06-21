import axios from 'axios';


const backend_api = axios.create({
    baseURL: 'http://localhost:8000', // Update with your backend URL 
});

const esp32 = axios.create({
    baseURL: 'http://192.168.113.200', // Update with your ESP32 URL
});

export { backend_api, esp32 };