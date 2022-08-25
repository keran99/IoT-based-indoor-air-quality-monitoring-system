'''
' This code takes as input the data contained in influxdb and subsequently
' applies the ARIMA model in order to predict future values. The user must
' enter the following data when requested: field to predict, the sample_frequency
' of the IoT device and the number of predictions to be made.
'''

import influxdb_client
from influxdb_client.client.write_api import SYNCHRONOUS
import paho.mqtt.client as paho
import pandas as pd
import matplotlib.pyplot as plt
import math
import time
from datetime import datetime
from statsmodels.tsa.stattools import adfuller
from statsmodels.graphics import tsaplots
from statsmodels.tsa.arima.model import ARIMA
from sklearn.metrics import mean_squared_error

# MQTT
def on_publish(client, userdata, result):  # create function for callback
    print("data published \n")
    pass

broker = ""
port =

# Access to Inflaxdb
bucket = ""
org = ""
token = ""
url=""

client = influxdb_client.InfluxDBClient(
   url=url,
   token=token,
   org=org
)

# User inputs
print('1 - TEMPERATURE')
print('2 - HUMIDITY')
print('3 - GAS')
topic = input("Enter : ")
delay = input("Enter the SAMPLE FREQUENCY: ")
num = input("Enter the number of values to forecasting: ")
num = int(num)

query_api = client.query_api()

query = ""
mqttTopic=""
if topic == '1':
    query = ' from(bucket:"ProgettoIoT")\
    |> range(start: -10m)\
    |> filter(fn: (r) => r["ID"] == "ESP32Client-01")\
    |> filter(fn: (r) => r["_field"] == "TEMPERATURE")'
    mqttTopic='TemperatureForecast'

elif topic == '2':
    query = ' from(bucket:"ProgettoIoT")\
    |> range(start: -10m)\
    |> filter(fn: (r) => r["ID"] == "ESP32Client-01")\
    |> filter(fn: (r) => r["_field"] == "HUMIDITY")'
    mqttTopic='HumidityForecast'

else:
    query = ' from(bucket:"ProgettoIoT")\
    |> range(start: -10m)\
    |> filter(fn: (r) => r["ID"] == "ESP32Client-01")\
    |> filter(fn: (r) => r["_field"] == "GAS")'
    mqttTopic='GasForecast'

result = query_api.query(org=org, query=query)
results = []
for table in result:
    for record in table.records:
        results.append(( record.get_value(), record.get_time()))

df = pd.DataFrame(data=results)

df[1]= pd.to_datetime(df[1], infer_datetime_format=True)
df = df.set_index(1, inplace=False)

train = df[0]
print(train)

result = adfuller(train)
print('ADF Statistic: %f' % result[0])
print('p-value: %f' % result[1])

fig = tsaplots.plot_acf(train, lags=10)
plt.show()

fig = tsaplots.plot_pacf(train, lags=10)
plt.show()

history = [x for x in train]
predictions = list()

# MQTT
client1 = paho.Client("control1")  # create client object
client1.on_publish = on_publish  # assign function to callback
client1.username_pw_set("", "")
client1.connect(broker, port)  # establish connection

# ARIMA model and publish data
for t in range(num):
    model = ARIMA(history, order=(1,1,1))
    model_fit = model.fit()
    output = model_fit.forecast()
    yest = output[0]
    predictions.append(yest)
    print('predicted=%f' % yest)
    ret = client1.publish(mqttTopic, yest)
    time.sleep(int(delay))
