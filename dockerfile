FROM python:3.9-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

# Environment variables
ENV PORT=8080
ENV SERIAL_PORT=""
ENV BAUD_RATE=9600

EXPOSE 8080

CMD ["uvicorn", "water_monitor_server:app", "--host", "0.0.0.0", "--port", "80"]