#pragma once
#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
    <title>ESP Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
        html {
            font-family: Arial, Helvetica, sans-serif;
            text-align: center;
        }
        h1 {
            font-size: 1.8rem;
            color: white;
        }
        h2{
            font-size: 1.5rem;
            font-weight: bold;
            color: #143642;
            margin-top: 10px;
        }
        .topnav {
            overflow: hidden;
            background-color: #143642;
        }
        body {
            margin: 0;
        }
        .content {
            padding: 30px;
            max-width: 1200px;
            margin: 0 auto;
        }
        .card {
            background-color: #F8F7F9;;
            box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
            padding-top:10px;
            padding-bottom:20px;
            margin-bottom: 20px
        }
        .button {
            padding: 15px 50px;
            font-size: 24px;
            text-align: center;
            outline: none;
            color: #fff;
            background-color: #0f8b8d;
            border: none;
            border-radius: 5px;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            -khtml-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
            -webkit-tap-highlight-color: rgba(0,0,0,0);
        }
        /*.button:hover {background-color: #0f8b8d}*/
        .button:active {
            background-color: #0f8b8d;
            box-shadow: 2 2px #CDCDCD;
            transform: translateY(2px);
        }
        .state {
            font-size: 3rem;
            color:#8c8c8c;
            font-weight: bold;
            margin: 0;
        }
        .smaller-state {
            font-size: 2rem;
        }
        .horizontal-spacer {
            height: 2000px;
        }
        .horizontal-layout {
            display: flex;
            justify-content: center;
            margin-top: 10px;
            flex-wrap: wrap;
            gap: 10px;
        }
        .small-button {
            padding: 5px 10px;
            font-size: 14px;
            text-align: center;
            outline: none;
            color: #fff;
            background-color: #0f8b8d;
            border: none;
            border-radius: 5px;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            -khtml-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
            -webkit-tap-highlight-color: rgba(0,0,0,0);
            margin-right: 5px;
        }
        .small-button:active {
            background-color: #0f8b8d;
            box-shadow: 2 2px #CDCDCD;
            transform: translateY(2px);
        }
        .chart {
            width: 100%;
            height: 400px;
        }
    </style>
    <title>ESP Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <script type="text/javascript">
        var gateway = `ws://${window.location.hostname}/ws`;
        var websocket;
        var last_server_msg_time = Date.now();
        var selectedTimeRange = 60000; 
        var raw_light_data = [];
        var last_helligkeit_update;
        var format = 'HH:mm:ss';
        
        // initialize data table for chart
        google.charts.load('current', {packages: ['corechart']});
        google.charts.setOnLoadCallback(drawLightChart);

        window.addEventListener('resize', function(event){
            drawLightChart();
        });
        
        window.addEventListener('load', onLoad);
        function onLoad(event) {
            initWebSocket();
            initButton();
            setInterval(function() {
                if (Date.now() - last_server_msg_time > 1500) {
                    document.getElementById('server_status').innerHTML = 'Verbindung getrennt!';
                }                
            }, 1000); 
        }
        function initButton() {
            document.getElementById('oneMin').addEventListener('click', getOneMinuteDataHelligkeit);
            document.getElementById('oneHour').addEventListener('click', getOneHourDataHelligkeit);
            document.getElementById('twelveHours').addEventListener('click', getTwelveHourDataHelligkeit);
            document.getElementById('oneDay').addEventListener('click', getOneDayDataHelligkeit);
            document.getElementById('oneWeek').addEventListener('click', getOneWeekDataHelligkeit);
        }

        function initWebSocket() {
            console.log('Trying to open a WebSocket connection...');
            websocket = new WebSocket(gateway);
            websocket.onopen    = onOpen;
            websocket.onclose   = onClose;
            websocket.onmessage = onMessage; 
        }
        function onOpen(event) {
            console.log('Connection opened');
            getOneMinuteDataHelligkeit();
        }
        function onClose(event) {
            console.log('Connection closed');
            initWebSocket();
        }
        function onMessage(event) {
            last_server_msg_time = Date.now();
            document.getElementById('server_status').innerHTML = 'Verbunden';

            var message = event.data;
            var state = message.split(':');
            if (state[0] == 'H') {
                document.getElementById('pot_state').innerHTML = state[1];
                var waittime = selectedTimeRange / 60;
                if (Date.now() - last_helligkeit_update > waittime) {
                    raw_light_data.push([new Date(), parseInt(state[1])]);
                    if (raw_light_data.length > 60) {
                        raw_light_data.shift();
                    }
                    drawLightChart();
                    last_helligkeit_update = Date.now();
                }
            } else if (state[0] == 'DH') {
                raw_light_data = [];
                var values = state[1].split(',');
                var time_since_last_update = parseInt(values[0]) * 100;
                for (var i = 1; i < values.length-1; i++) {
                    raw_light_data.push([new Date(new Date().getTime() - selectedTimeRange + (i + 60-values.length+2) * selectedTimeRange / 60 - time_since_last_update), parseInt(values[i])]);
                }
                last_helligkeit_update = Date.now() - time_since_last_update;
                drawLightChart();
            }
        }

        function getOneMinuteDataHelligkeit() {
            websocket.send('HM');
            selectedTimeRange = 60000;
            format = 'HH:mm:ss';
        }

        function getOneHourDataHelligkeit() {
            websocket.send('HH');
            selectedTimeRange = 3600000;
            format = 'HH:mm';
        }

        function getOneDayDataHelligkeit() {
            websocket.send('HD');
            selectedTimeRange = 86400000;
            format = 'HH:mm';
        }

        function getTwelveHourDataHelligkeit() {
            websocket.send('HT');
            selectedTimeRange = 43200000;
            format = 'HH:mm';
        }

        function getOneWeekDataHelligkeit() {
            websocket.send('HW');
            selectedTimeRange = 604800000;
            format = 'dd/MM HH:mm';
        }

        function drawLightChart() {   
            var data = new google.visualization.DataTable();
            data.addColumn('datetime', 'Zeit');
            data.addColumn('number', 'Helligkeit');
            data.addRows(raw_light_data);

            var options = {
                curveType: 'function',
                legend: { position: 'bottom' },
                colors: ['#0f8b8d'],
                backgroundColor: '#F8F7F9',
                vAxis: {
                    minValue: 0,
                    maxValue: 100,
                    viewWindow: {
                        min: 0,
                        max: 100
                    }
                },
                hAxis: {
                    format: format,
                    minValue: new Date(new Date().getTime() - selectedTimeRange),
                    viewWindow: {
                        min: new Date(new Date().getTime() - selectedTimeRange),
                        max: new Date()
                    }
                }
            };

            var chart = new google.visualization.LineChart(document.getElementById('curve_chart'));

            chart.draw(data, options);
        }
    </script>
</head>
<body>
    <div class="topnav">
        <h1>ESP WebSocket Server</h1>
    </div>
    <div class="content">
        <div class="card">
            <h2>Server Status</h2>
            <p class="state smaller-state"><span id="server_status">Verbunden</span></p>
        </div>
        <div class="card">
            <h2>Helligkeit</h2>
            <p class="state"><span id="pot_state">%POT_STATE%</span>&#37</p>
            <div id="curve_chart" class="chart"></div>
            <div class="horizontal-layout" >
                <button class="small-button" id="oneMin">1 Minute</button>
                <button class="small-button" id="oneHour">1 Stunde</button>
                <button class="small-button" id="twelveHours">12 Stunden</button>
                <button class="small-button" id="oneDay">1 Tag</button>
                <button class="small-button" id="oneWeek">1 Woche</button>
            </div>
        </div>
    </div>
    <div class="horizontal-spacer"></div>
</body>
</html>
)rawliteral";