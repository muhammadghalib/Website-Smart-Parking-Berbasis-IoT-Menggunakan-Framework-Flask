from flask import Flask, request, jsonify, render_template
from flask_caching import Cache
import threading
import time
import orjson

app = Flask(__name__)

cache_config = {
    "CACHE_TYPE": "SimpleCache",
    "CACHE_DEFAULT_TIMEOUT": 60,
}
app.config.from_mapping(cache_config)
cache = Cache(app)

lock = threading.Lock()

app.config["JSONIFY_PRETTYPRINT_REGULAR"] = False

@app.after_request
def set_json_response(response):
    if response.content_type == "application/json":
        response.data = orjson.dumps(response.json)
    return response

@app.route("/")
def home():
    return render_template("index.html")

@app.route("/update-data", methods=["POST"])
def update_slot():
    data = request.get_json()
    if not data:
        return jsonify({"message": "Data tidak valid"}), 400

    with lock:
        cache.set("data_storage", data)
        cache.set("last_updated", time.time())

    return jsonify({"message": "Data diterima"}), 200

@app.route("/data", methods=["GET"])
def get_data():
    current_time = time.time()

    with lock:
        last_updated = cache.get("last_updated")

    if last_updated is None or current_time - last_updated > 5:
        return jsonify({"status": "ESP32 Not Available"}), 200

    # Ambil data dari cache
    with lock:
        data_storage = cache.get("data_storage")
    
    if not data_storage:
        return jsonify({"message": "Data tidak tersedia"}), 500

    return jsonify(data_storage), 200

if __name__ == "__main__":
    app.run(debug=False, host="0.0.0.0", port=5000, threaded=True)

