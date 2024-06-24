from flask import Flask, jsonify, render_template
from flask_sqlalchemy import SQLAlchemy
from flask_socketio import SocketIO, emit
import psutil

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'mysql+mysqlconnector://root:1234@localhost/SO2P1'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)
socketio = SocketIO(app)

class Datos(db.Model):
    __tablename__ = 'datos'
    pid = db.Column(db.String(100), primary_key=True)
    nombre = db.Column(db.String(100))
    llamada = db.Column(db.String(100))
    size = db.Column(db.String(100))
    fecha = db.Column(db.String(100))

@app.route('/api/datos', methods=['GET'])
def get_datos():
    datos = Datos.query.all()
    result = []
    for dato in datos:
        result.append({
            'pid': dato.pid,
            'nombre': dato.nombre,
            'llamada': dato.llamada,
            'size': max(0, int(dato.size)),  # Si el resultado es negativo, muestra 0
            'porcentaje_memoria': get_memory_percentage(dato.pid)
        })
    return jsonify(result)

@app.route('/api/calls', methods=['GET'])
def get_calls():
    datos = Datos.query.all()
    result = []
    for dato in datos:
        result.append({
            'pid': dato.pid,
            'llamada': dato.llamada,
            'size': dato.size,
            'fecha': dato.fecha
        })
    return jsonify(result)

def get_memory_percentage(pid):
    total_virtual_memory = psutil.virtual_memory().total
    try:
        process = psutil.Process(int(pid))
        process_memory = process.memory_info().vms  # Obtener memoria virtual del proceso
        if total_virtual_memory > 0:
            return (process_memory / total_virtual_memory) * 100
        return 0
    except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
        return 0

@app.route('/')
def index():
    return render_template('index.html')

@socketio.on('connect')
def handle_connect():
    print('Client connected')

@socketio.on('disconnect')
def handle_disconnect():
    print('Client disconnected')

@socketio.on('update_data')
def handle_update_data(data):
    emit('update_data', data, broadcast=True)

if __name__ == '__main__':
    socketio.run(app)
