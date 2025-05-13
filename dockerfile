FROM python:3.9-slim

WORKDIR /app

# Copy only the required files
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY water-monitor-server.py .

# Environment variables
ENV PORT=8080
ENV SERIAL_PORT=""
ENV BAUD_RATE=9600

EXPOSE 8080

# Use the PORT env variable consistently
CMD ["uvicorn", "water-monitor-server:app", "--host", "0.0.0.0", "--port", "8080"]