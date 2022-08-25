'''
' This code obtains the data relating to the temperature,
' humidity and pressure of a city given as input by the user.
' To do this, he makes a GET request to openWeather.
' subsequently sent this data to influxDB through the MQTT protocol.
'''

import paho.mqtt.client as paho
import requests as requests
import time

# MQTT
def on_publish(client, userdata, result):  # create function for callback
    print("data published \n")
    pass

broker = ""
port =

api_key = ""
base_url = "http://api.openweathermap.org/data/2.5/weather?"
city_name = input("Enter city name : ")
a = 0

# Send request to OpenWeather
complete_url = base_url + "appid=" + api_key + "&q=" + city_name
response = requests.get(complete_url)
x = response.json()

if x["cod"] != "404":
    while a < 1:
        y = x["main"]
        current_temperature = y["temp"]
        current_pressure = y["pressure"]
        current_humidity = y["humidity"]
        z = x["weather"]
        weather_description = z[0]["description"]

        # print following values
        print(" Temperature (in kelvin unit) = " +
              str(current_temperature) +
              "\n atmospheric pressure (in hPa unit) = " +
              str(current_pressure) +
              "\n humidity (in percentage) = " +
              str(current_humidity) +
              "\n description = " +
              str(weather_description))

        current_temperature = round(current_temperature - 273.15, 2);

        # MQTT publish data
        client1 = paho.Client("control1")  # create client object
        client1.on_publish = on_publish  # assign function to callback
        client1.username_pw_set("", "")
        client1.connect(broker, port)  # establish connection

        ret = client1.publish("TEMPERATURE", current_temperature)
        ret2 = client1.publish("HUMIDITY", current_humidity)
        ret3 = client1.publish("PRESSURE", current_pressure)
        time.sleep(5)
else:
    print(" City Not Found ")
