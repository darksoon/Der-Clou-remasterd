
  var Module = typeof Module !== 'undefined' ? Module : {};

  if (!Module['expectedDataFileDownloads']) Module['expectedDataFileDownloads'] = 0;
  Module['expectedDataFileDownloads']++;
  (() => {
    // Do not attempt to redownload the virtual filesystem data when in a pthread or a Wasm Worker context.
    var isPthread = typeof ENVIRONMENT_IS_PTHREAD !== 'undefined' && ENVIRONMENT_IS_PTHREAD;
    var isWasmWorker = typeof ENVIRONMENT_IS_WASM_WORKER !== 'undefined' && ENVIRONMENT_IS_WASM_WORKER;
    if (isPthread || isWasmWorker) return;
    var isNode = globalThis.process && globalThis.process.versions && globalThis.process.versions.node && globalThis.process.type != 'renderer';
    async function loadPackage(metadata) {

      var PACKAGE_PATH = '';
      if (typeof window === 'object') {
        PACKAGE_PATH = window['encodeURIComponent'](window.location.pathname.substring(0, window.location.pathname.lastIndexOf('/')) + '/');
      } else if (typeof process === 'undefined' && typeof location !== 'undefined') {
        // web worker
        PACKAGE_PATH = encodeURIComponent(location.pathname.substring(0, location.pathname.lastIndexOf('/')) + '/');
      }
      var PACKAGE_NAME = 'voices.data';
      var REMOTE_PACKAGE_BASE = 'voices.data';
      var REMOTE_PACKAGE_NAME = Module['locateFile'] ? Module['locateFile'](REMOTE_PACKAGE_BASE, '') : REMOTE_PACKAGE_BASE;
      var REMOTE_PACKAGE_SIZE = metadata['remote_package_size'];

      async function fetchRemotePackage(packageName, packageSize) {
        if (isNode) {
          var contents = require('fs').readFileSync(packageName);
          return new Uint8Array(contents).buffer;
        }
        if (!Module['dataFileDownloads']) Module['dataFileDownloads'] = {};
        try {
          var response = await fetch(packageName);
        } catch (e) {
          throw new Error(`Network Error: ${packageName}`, {e});
        }
        if (!response.ok) {
          throw new Error(`${response.status}: ${response.url}`);
        }

        const chunks = [];
        const headers = response.headers;
        const total = Number(headers.get('Content-Length') || packageSize);
        let loaded = 0;

        Module['setStatus'] && Module['setStatus']('Downloading data...');
        const reader = response.body.getReader();

        while (1) {
          var {done, value} = await reader.read();
          if (done) break;
          chunks.push(value);
          loaded += value.length;
          Module['dataFileDownloads'][packageName] = {loaded, total};

          let totalLoaded = 0;
          let totalSize = 0;

          for (const download of Object.values(Module['dataFileDownloads'])) {
            totalLoaded += download.loaded;
            totalSize += download.total;
          }

          Module['setStatus'] && Module['setStatus'](`Downloading data... (${totalLoaded}/${totalSize})`);
        }

        const packageData = new Uint8Array(chunks.map((c) => c.length).reduce((a, b) => a + b, 0));
        let offset = 0;
        for (const chunk of chunks) {
          packageData.set(chunk, offset);
          offset += chunk.length;
        }
        return packageData.buffer;
      }

      var fetchPromise;
      var fetched = Module['getPreloadedPackage'] && Module['getPreloadedPackage'](REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);

      if (!fetched) {
        // Note that we don't use await here because we want to execute the
        // the rest of this function immediately.
        fetchPromise = fetchRemotePackage(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);
      }

    async function runWithFS(Module) {

      function assert(check, msg) {
        if (!check) throw new Error(msg);
      }
Module['FS_createPath']("/", "gamedata", true, true);
Module['FS_createPath']("/gamedata", "PLUS", true, true);
Module['FS_createPath']("/gamedata/PLUS", "AMBIENT", true, true);
Module['FS_createPath']("/gamedata/PLUS", "VOICE", true, true);
Module['FS_createPath']("/gamedata/PLUS", "WEATHER", true, true);

      async function processPackageData(arrayBuffer) {
        assert(arrayBuffer, 'Loading data file failed.');
        assert(arrayBuffer.constructor.name === ArrayBuffer.name, 'bad input to processPackageData ' + arrayBuffer.constructor.name);
        var byteArray = new Uint8Array(arrayBuffer);
        var curr;
        // Reuse the bytearray from the XHR as the source for file reads.
          for (var file of metadata['files']) {
            var name = file['filename'];
            var data = byteArray.subarray(file['start'], file['end']);
            // canOwn this data in the filesystem, it is a slice into the heap that will never change
        Module['FS_createDataFile'](name, null, data, true, true, true);
          }
          Module['removeRunDependency']('datafile_voices.data');
      }
      Module['addRunDependency']('datafile_voices.data');

      if (!Module['preloadResults']) Module['preloadResults'] = {};

      Module['preloadResults'][PACKAGE_NAME] = {fromCache: false};
      if (!fetched) {
        fetched = await fetchPromise;
      }
      await processPackageData(fetched);

    }
    // Detect whether the module JS file has already been loaded.
    if (Module['FS_createPath']) {
      runWithFS(Module);
    } else {
      if (!Module['preRun']) Module['preRun'] = [];
      Module['preRun'].push(runWithFS); // FS is not initialized yet, wait for it
    }

    }
    loadPackage({"files": [{"filename": "/gamedata/PLUS/AMBIENT/GARAGE.WAV", "start": 0, "end": 1173104}, {"filename": "/gamedata/PLUS/AMBIENT/HOTEL.WAV", "start": 1173104, "end": 2346208}, {"filename": "/gamedata/PLUS/AMBIENT/LOBBY.WAV", "start": 2346208, "end": 3519312}, {"filename": "/gamedata/PLUS/AMBIENT/PARK.WAV", "start": 3519312, "end": 4692416}, {"filename": "/gamedata/PLUS/AMBIENT/POLICE.WAV", "start": 4692416, "end": 5865520}, {"filename": "/gamedata/PLUS/AMBIENT/PUB.WAV", "start": 5865520, "end": 7038624}, {"filename": "/gamedata/PLUS/AMBIENT/SHOP.WAV", "start": 7038624, "end": 8211728}, {"filename": "/gamedata/PLUS/AMBIENT/STATION.WAV", "start": 8211728, "end": 9384832}, {"filename": "/gamedata/PLUS/AMBIENT/STREET.WAV", "start": 9384832, "end": 10557936}, {"filename": "/gamedata/PLUS/VOICE/5TH_GLUDO_0.WAV", "start": 10557936, "end": 10776716}, {"filename": "/gamedata/PLUS/VOICE/5TH_GLUDO_1.WAV", "start": 10776716, "end": 11006080}, {"filename": "/gamedata/PLUS/VOICE/5TH_GLUDO_2.WAV", "start": 11006080, "end": 11111964}, {"filename": "/gamedata/PLUS/VOICE/5TH_GLUDO_3.WAV", "start": 11111964, "end": 11291936}, {"filename": "/gamedata/PLUS/VOICE/5TH_GLUDO_4.WAV", "start": 11291936, "end": 11341372}, {"filename": "/gamedata/PLUS/VOICE/5TH_GLUDO_5.WAV", "start": 11341372, "end": 11475480}, {"filename": "/gamedata/PLUS/VOICE/ARREST_GLUDO_0.WAV", "start": 11475480, "end": 11591948}, {"filename": "/gamedata/PLUS/VOICE/ARREST_GLUDO_1.WAV", "start": 11591948, "end": 11817784}, {"filename": "/gamedata/PLUS/VOICE/A_CALL_FOR_YOU.WAV", "start": 11817784, "end": 12064788}, {"filename": "/gamedata/PLUS/VOICE/BRIGGS_BRIGGS_1.WAV", "start": 12064788, "end": 12946832}, {"filename": "/gamedata/PLUS/VOICE/BRIGGS_BRIGGS_2.WAV", "start": 12946832, "end": 13469020}, {"filename": "/gamedata/PLUS/VOICE/BRIGGS_BRIGGS_3.WAV", "start": 13469020, "end": 13980624}, {"filename": "/gamedata/PLUS/VOICE/BRIGGS_BRIGGS_4.WAV", "start": 13980624, "end": 14735660}, {"filename": "/gamedata/PLUS/VOICE/BRIGGS_BRIGGS_5.WAV", "start": 14735660, "end": 15032056}, {"filename": "/gamedata/PLUS/VOICE/CALL_BRIGGS_0.WAV", "start": 15032056, "end": 15642444}, {"filename": "/gamedata/PLUS/VOICE/CALL_BRIGGS_1.WAV", "start": 15642444, "end": 15868280}, {"filename": "/gamedata/PLUS/VOICE/DANNER_1.WAV", "start": 15868280, "end": 16196428}, {"filename": "/gamedata/PLUS/VOICE/DANNER_21.WAV", "start": 16196428, "end": 17374824}, {"filename": "/gamedata/PLUS/VOICE/DANNER_22.WAV", "start": 17374824, "end": 18517940}, {"filename": "/gamedata/PLUS/VOICE/DANNER_23.WAV", "start": 18517940, "end": 19766896}, {"filename": "/gamedata/PLUS/VOICE/DART_GRULL_0.WAV", "start": 19766896, "end": 19996260}, {"filename": "/gamedata/PLUS/VOICE/DART_GRULL_1.WAV", "start": 19996260, "end": 20045696}, {"filename": "/gamedata/PLUS/VOICE/DART_GRULL_2.WAV", "start": 20045696, "end": 20193916}, {"filename": "/gamedata/PLUS/VOICE/DEALER_0.WAV", "start": 20193916, "end": 20659656}, {"filename": "/gamedata/PLUS/VOICE/DEALER_1.WAV", "start": 20659656, "end": 21188900}, {"filename": "/gamedata/PLUS/VOICE/DEALER_2.WAV", "start": 21188900, "end": 21524104}, {"filename": "/gamedata/PLUS/VOICE/DEALER_IS_AFRAID.WAV", "start": 21524104, "end": 21735828}, {"filename": "/gamedata/PLUS/VOICE/FREE_TICKET.WAV", "start": 21735828, "end": 22826024}, {"filename": "/gamedata/PLUS/VOICE/GROVE_SABIEN_0.WAV", "start": 22826024, "end": 22917796}, {"filename": "/gamedata/PLUS/VOICE/GROVE_SABIEN_1.WAV", "start": 22917796, "end": 23027208}, {"filename": "/gamedata/PLUS/VOICE/GROVE_SABIEN_2.WAV", "start": 23027208, "end": 23182484}, {"filename": "/gamedata/PLUS/VOICE/GROVE_SABIEN_3.WAV", "start": 23182484, "end": 23313064}, {"filename": "/gamedata/PLUS/VOICE/HOTEL_ANT_11.WAV", "start": 23313064, "end": 23401308}, {"filename": "/gamedata/PLUS/VOICE/HOTEL_ANT_12.WAV", "start": 23401308, "end": 23803544}, {"filename": "/gamedata/PLUS/VOICE/HOTEL_ANT_13.WAV", "start": 23803544, "end": 24428044}, {"filename": "/gamedata/PLUS/VOICE/HOTEL_ANT_14.WAV", "start": 24428044, "end": 24667992}, {"filename": "/gamedata/PLUS/VOICE/HOTEL_ANT_21.WAV", "start": 24667992, "end": 25133732}, {"filename": "/gamedata/PLUS/VOICE/HOTEL_ANT_22.WAV", "start": 25133732, "end": 25698256}, {"filename": "/gamedata/PLUS/VOICE/HOTEL_ANT_31.WAV", "start": 25698256, "end": 26065212}, {"filename": "/gamedata/PLUS/VOICE/I_LL_FETCH_THE_KEY.WAV", "start": 26065212, "end": 26209904}, {"filename": "/gamedata/PLUS/VOICE/KEN_PLAN.WAV", "start": 26209904, "end": 29134660}, {"filename": "/gamedata/PLUS/VOICE/POLI_ANT_11.WAV", "start": 29134660, "end": 29378136}, {"filename": "/gamedata/PLUS/VOICE/POLI_ANT_12.WAV", "start": 29378136, "end": 29526356}, {"filename": "/gamedata/PLUS/VOICE/POLI_ANT_13.WAV", "start": 29526356, "end": 29738080}, {"filename": "/gamedata/PLUS/VOICE/POLI_ANT_21.WAV", "start": 29738080, "end": 29918052}, {"filename": "/gamedata/PLUS/VOICE/POLI_ANT_51.WAV", "start": 29918052, "end": 30105080}, {"filename": "/gamedata/PLUS/VOICE/RAID_POLICE_0.WAV", "start": 30105080, "end": 30299164}, {"filename": "/gamedata/PLUS/VOICE/RAID_POLICE_1.WAV", "start": 30299164, "end": 30383880}, {"filename": "/gamedata/PLUS/VOICE/READ_TIMES_GLUDO.WAV", "start": 30383880, "end": 30535628}, {"filename": "/gamedata/PLUS/VOICE/SAILOR_GLUDO_0.WAV", "start": 30535628, "end": 30814384}, {"filename": "/gamedata/PLUS/VOICE/SAILOR_GLUDO_1.WAV", "start": 30814384, "end": 31160172}, {"filename": "/gamedata/PLUS/VOICE/SAILOR_GLUDO_2.WAV", "start": 31160172, "end": 31823480}, {"filename": "/gamedata/PLUS/VOICE/ST1_BRIGGS_0.WAV", "start": 31823480, "end": 31883500}, {"filename": "/gamedata/PLUS/VOICE/ST1_BRIGGS_1.WAV", "start": 31883500, "end": 32045832}, {"filename": "/gamedata/PLUS/VOICE/ST1_BRIGGS_2.WAV", "start": 32045832, "end": 32536268}, {"filename": "/gamedata/PLUS/VOICE/ST1_BRIGGS_3.WAV", "start": 32536268, "end": 32726824}, {"filename": "/gamedata/PLUS/VOICE/ST1_BRIGGS_4.WAV", "start": 32726824, "end": 33178452}, {"filename": "/gamedata/PLUS/VOICE/ST_10_SABIEN.WAV", "start": 33178452, "end": 33481904}, {"filename": "/gamedata/PLUS/VOICE/ST_11_ALL_0.WAV", "start": 33481904, "end": 33591316}, {"filename": "/gamedata/PLUS/VOICE/ST_12_AGENT.WAV", "start": 33591316, "end": 33841848}, {"filename": "/gamedata/PLUS/VOICE/ST_14_AGENT_0.WAV", "start": 33841848, "end": 34032404}, {"filename": "/gamedata/PLUS/VOICE/ST_15_ALLEN_0.WAV", "start": 34032404, "end": 34635736}, {"filename": "/gamedata/PLUS/VOICE/ST_18_SABIEN_0.WAV", "start": 34635736, "end": 34985052}, {"filename": "/gamedata/PLUS/VOICE/ST_18_SABIEN_1.WAV", "start": 34985052, "end": 36152864}, {"filename": "/gamedata/PLUS/VOICE/ST_24_BRIGGS_0.WAV", "start": 36152864, "end": 36286972}, {"filename": "/gamedata/PLUS/VOICE/ST_24_BRIGGS_1.WAV", "start": 36286972, "end": 37207824}, {"filename": "/gamedata/PLUS/VOICE/ST_24_BRIGGS_2.WAV", "start": 37207824, "end": 37684148}, {"filename": "/gamedata/PLUS/VOICE/ST_24_BRIGGS_3.WAV", "start": 37684148, "end": 38192224}, {"filename": "/gamedata/PLUS/VOICE/ST_25_BRIGGS_0.WAV", "start": 38192224, "end": 38291052}, {"filename": "/gamedata/PLUS/VOICE/ST_25_PATE_0.WAV", "start": 38291052, "end": 38407520}, {"filename": "/gamedata/PLUS/VOICE/ST_26_PATE_0.WAV", "start": 38407520, "end": 39303676}, {"filename": "/gamedata/PLUS/VOICE/ST_3_SABIEN_0.WAV", "start": 39303676, "end": 39388392}, {"filename": "/gamedata/PLUS/VOICE/ST_6_BEN_0.WAV", "start": 39388392, "end": 39582476}, {"filename": "/gamedata/PLUS/VOICE/ST_8_SABIEN_0.WAV", "start": 39582476, "end": 39931792}, {"filename": "/gamedata/PLUS/VOICE/ST_9_BRIGGS_0.WAV", "start": 39931792, "end": 40411644}, {"filename": "/gamedata/PLUS/WEATHER/RAIN.WAV", "start": 40411644, "end": 41584748}, {"filename": "/gamedata/PLUS/WEATHER/THUNDER1.WAV", "start": 41584748, "end": 41849392}, {"filename": "/gamedata/PLUS/WEATHER/THUNDER2.WAV", "start": 41849392, "end": 42069936}], "remote_package_size": 42069936});

  })();
