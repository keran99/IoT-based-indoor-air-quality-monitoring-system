'''
' This code takes the data contained in influxdb as input.
' This data is divided into two groups: train and test.
' First of all, a training phase is carried out and subsequently
' the dataframe tests are given as input to the ARIMA model.
' Finally, MSE and RMSE are calculated.
'''

import influxdb_client
from influxdb_client.client.write_api import SYNCHRONOUS
import pandas as pd
import matplotlib.pyplot as plt
import math
from datetime import datetime
from statsmodels.tsa.stattools import adfuller
from statsmodels.graphics import tsaplots
from statsmodels.tsa.arima.model import ARIMA
from sklearn.metrics import mean_squared_error

# Access to Inflaxdb
bucket = ""
org = ""
token = ""
# Store the URL of your InfluxDB instance
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

query_api = client.query_api()
query = ""
if topic == '1':
    query = ' from(bucket:"ProgettoIoT")\
    |> range(start: -3h)\
    |> filter(fn: (r) => r["ID"] == "ESP32Client-01")\
    |> filter(fn: (r) => r["_field"] == "TEMPERATURE")'

elif topic == '2':
    query = ' from(bucket:"ProgettoIoT")\
    |> range(start: -3h)\
    |> filter(fn: (r) => r["ID"] == "ESP32Client-01")\
    |> filter(fn: (r) => r["_field"] == "HUMIDITY")'

else:
    query = ' from(bucket:"ProgettoIoT")\
    |> range(start: -3h)\
    |> filter(fn: (r) => r["ID"] == "ESP32Client-01")\
    |> filter(fn: (r) => r["_field"] == "GAS")'

result = query_api.query(org=org, query=query)
results = []
for table in result:
    for record in table.records:
        results.append(( record.get_value(), record.get_time()))

df = pd.DataFrame(data=results)
print(df)

df[1]= pd.to_datetime(df[1], infer_datetime_format=True)
df = df.set_index(1, inplace=False)
print(df)

nrows = (len(df.values))
splitPoint = int(nrows * 0.8)

train = df[0][:splitPoint]
test = df[0][splitPoint:]

print(train)

result = adfuller(train)
print('ADF Statistic: %f' % result[0])
print('p-value: %f' % result[1])

'''
trainNew = train.diff().dropna()

result = adfuller(trainNew)
print('ADF Statistic: %f' % result[0])
print('p-value: %f' % result[1])

fig = tsaplots.plot_acf(trainNew, lags=10)
plt.show()

fig = tsaplots.plot_pacf(trainNew, lags=10)
plt.show()
'''

fig = tsaplots.plot_acf(train, lags=10)
plt.show()

fig = tsaplots.plot_pacf(train, lags=10)
plt.show()

history = [x for x in train]
predictions = list()

# ARIMA model
for t in range(len(test)):
    model = ARIMA(history, order=(1,1,1))
    model_fit = model.fit()
    output = model_fit.forecast()
    yest = output[0]
    predictions.append(yest)
    obs = test[t]
    history.append(obs)
    print('predicted=%f, exprected=%f' % (yest, obs))

mse = mean_squared_error(test, predictions)
print('Test MSE: %.3f' % mse)
rmse = math.sqrt(mean_squared_error(test, predictions))
print('Test RMSE: %.3f' % rmse)

df2 = pd.DataFrame(predictions)
df2.set_index(test.index, inplace=True)

plt.plot(test, color='blue')
plt.plot(df2, color='red')
plt.show()
