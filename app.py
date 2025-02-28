from flask import Flask, request, jsonify, render_template, Response
from datetime import datetime, timedelta
import csv
import os
import json
import time

app = Flask(__name__)

# Ensure CSV file exists with headers
csv_file = 'files/scores.csv'
os.makedirs(os.path.dirname(csv_file), exist_ok=True)
if not os.path.exists(csv_file):
    with open(csv_file, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['score', 'timestamp'])

def read_scores():
    """Helper function to read and parse scores from CSV"""
    scores_list = []
    with open(csv_file, 'r', newline='') as f:
        reader = csv.DictReader(f, fieldnames=['score', 'timestamp'])
        next(reader)  # Skip header row
        for row in reader:
            scores_list.append({
                'score': int(row['score']),
                'timestamp': row['timestamp']
            })
    return scores_list

def sort_scores(scores_list, reverse_time=True):
    """Helper function to sort scores by score and timestamp"""
    time_multiplier = -1 if reverse_time else 1
    return sorted(
        scores_list,
        key=lambda x: (-x['score'], time_multiplier * datetime.fromisoformat(x['timestamp']).timestamp())
    )

@app.route('/score_stream')
def score_stream():
    def generate():
        while True:
            try:
                scores_list = read_scores()
                sorted_scores = sort_scores(scores_list)
                top_scores = sorted_scores[:10]
                
                data = {
                    "top_scores": top_scores,
                    "count": len(top_scores)
                }
                
                yield f"data: {json.dumps(data)}\n\n"
            except Exception as e:
                print(f"Error: {e}")
            
            time.sleep(1)

    return Response(generate(), mimetype='text/event-stream')

@app.route('/')
def leaderboard():
    return render_template('daily_scores.html')

@app.route('/game_result', methods=['POST'])
def receive_game_result():
    data = request.get_json()

    if not data:
        return jsonify({
            "message": "No game data received yet", 
            "timestamp": datetime.utcnow().isoformat()
        }), 200

    score = data.get("score", 0)
    timestamp = datetime.utcnow().isoformat()

    with open(csv_file, 'a', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([score, timestamp])

    return jsonify({
        "message": "Data received successfully", 
        "score": score, 
        "timestamp": timestamp
    }), 200

@app.route('/top_scores', methods=['GET'])
def get_top_scores():
    try:
        scores_list = read_scores()
        sorted_scores = sort_scores(scores_list, reverse_time=False)
        top_scores = sorted_scores[:10]
        
        return jsonify({
            "top_scores": top_scores,
            "count": len(top_scores)
        }), 200
    
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/get_daily_scores')
def get_daily_scores():
    date = request.args.get('date', datetime.now().strftime('%Y-%m-%d'))
    
    try:
        scores_list = read_scores()
        daily_scores = [
            score for score in scores_list
            if datetime.fromisoformat(score['timestamp']).strftime('%Y-%m-%d') == date
        ]
        
        sorted_scores = sort_scores(daily_scores)
        
        return jsonify({
            "scores": sorted_scores,
            "count": len(sorted_scores),
            "date": date
        }), 200
    
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/daily_scores')
def daily_scores_page():
    return render_template('daily_scores.html')

@app.route('/top_scores_page')
def top_scores_page():
    return render_template('top_scores.html')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
