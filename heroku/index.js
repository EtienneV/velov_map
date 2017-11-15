'use strict'

const express = require('express')
const app = express()
const bodyParser = require('body-parser')
const request = require('request')
const crontab = require('node-crontab');

app.set('port', (process.env.PORT || 5000))
app.use(bodyParser.urlencoded({extended: false}))
app.use(bodyParser.json())

/**
 * Render engine configuration
 */
app.set('views', __dirname + '/');
app.engine('ejs', require('ejs').renderFile);
app.set('view engine', 'ejs');

app.use(express.static(__dirname + '/'));

// Spin up the server
var server = app.listen(app.get('port'), function() {
    console.log('running on port', app.get('port'))
})

// Racine
app.get('/station/:idstation', function (req, res) {
	var id_station = req.params.idstation

	var info_stations = findStation(id_station)

	var json_reponse = JSON.stringify({
		velov: info_stations.free_bikes,
		places: info_stations.empty_slots
	})

    res.send(json_reponse);
})






var stations; // Contient les infos de toutes les stations, mis à jour toutes les minutes

const apiKey = '';
const contract_decaux = 'Lyon';

// On met à jour "manuellement la première fois"
request('https://api.jcdecaux.com/vls/v1/stations?contract='+contract_decaux+'&apiKey='+apiKey, function (error, response, body) { // On récupère les infos vélov
      if (!error && response.statusCode == 200) {
        var data_decaux = JSON.parse(body);

        var s = [];

        for(var i = 0 ; i < data_decaux.length ; i++) {
          s.push({
            "empty_slots": data_decaux[i].available_bike_stands,
            "extra": {
              "address": data_decaux[i].address,
              "banking": data_decaux[i].banking,
              "bonus": data_decaux[i].bonus,
              "last_update": data_decaux[i].last_update,
              "slots": data_decaux[i].bike_stands,
              "status": data_decaux[i].status,
              "uid": data_decaux[i].number
            },
            "free_bikes": data_decaux[i].available_bikes,
            "latitude": data_decaux[i].position.lat,
            "longitude": data_decaux[i].position.lng,
            "name": data_decaux[i].name
          })
        }

        stations = s;
      }
    })

crontab.scheduleJob("*/1 * * * *", function(){ //This will call this function every 1 minutes 

    request('https://api.jcdecaux.com/vls/v1/stations?contract='+contract_decaux+'&apiKey='+apiKey, function (error, response, body) { // On récupère les infos vélov
      if (!error && response.statusCode == 200) {
        var data_decaux = JSON.parse(body);

        var s = [];

        for(var i = 0 ; i < data_decaux.length ; i++) {
          s.push({
            "empty_slots": data_decaux[i].available_bike_stands,
            "extra": {
              "address": data_decaux[i].address,
              "banking": data_decaux[i].banking,
              "bonus": data_decaux[i].bonus,
              "last_update": data_decaux[i].last_update,
              "slots": data_decaux[i].bike_stands,
              "status": data_decaux[i].status,
              "uid": data_decaux[i].number
            },
            "free_bikes": data_decaux[i].available_bikes,
            "latitude": data_decaux[i].position.lat,
            "longitude": data_decaux[i].position.lng,
            "name": data_decaux[i].name
          })
        }

        stations = s;
      }
    })

});


// Obtenir les infos d'une station en fournissant son uid
function findStation (id_station) {
  var find_station_fct = function (station) {
    return station.extra.uid == this;
  };

  return stations.find(find_station_fct, id_station)
}