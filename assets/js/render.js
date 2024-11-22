
// 初始化地图
var map = L.map('map').setView([31.301623, 121.494740], 17);

// 添加地图图层
var mapLayers = {
    "空白":
    L.tileLayer(""),
    "osm地图": 
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
    }).addTo(map),
    "谷歌卫星":
    L.tileLayer('http://mt1.google.com/vt/lyrs=s&x={x}&y={y}&z={z}', {
        maxZoom: 20,
        minZoom: 3,
        subdomains: ['mt0','mt1','mt2','mt3']
    }),
    "高德卫星":
    L.tileLayer('https://webst0{s}.is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}', {
        maxZoom: 20,
        maxNativeZoom: 18,
        minZoom: 3,
        attribution: "高德地图 AutoNavi.com",
        subdomains: "1234"
    }),
    "综合":
    L.layerGroup([
        L.tileLayer('https://webst0{s}.is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}', {
            maxZoom: 20,
            maxNativeZoom: 18,
            minZoom: 3,
            attribution: "高德地图 AutoNavi.com",
            subdomains: "1234"
        }),
        L.tileLayer('https://webst0{s}.is.autonavi.com/appmaptile?style=8&x={x}&y={y}&z={z}', {
            maxZoom: 20,
            maxNativeZoom: 18,
            minZoom: 3,
            attribution: "高德地图 AutoNavi.com",
            subdomains: "1234",
            opacity: 0.5
        })
    ]),
}

var layerControl = L.control.layers(mapLayers, {}, {
    position: 'topright',
    collapsed: true
}).addTo(map);

{/* <bounds minlat="31.1029000" minlon="121.2897000" maxlat="31.3290000" maxlon="121.6969000"/> */}
var minlat = 31.1029000;
var minlon = 121.2897000;
var maxlat = 31.3290000;
var maxlon = 121.6969000;


const loadedChunks = {}; // 使用对象作为哈希表来存储已加载的块
const drawnItems = {}; // 用于存储在地图上绘制的图形

// 假设每个块的大小（以经纬度为单位）
const CHUNK_SIZE = 0.007;
const VIEW_SIZE = 0.014;

function getChunkId(lat, lng) {
    const chunkX = Math.floor(lat / CHUNK_SIZE);
    const chunkY = Math.floor(lng / CHUNK_SIZE);
    return `${chunkX}_${chunkY}`; // 以 "块X_块Y" 作为标识
}

function requestDataFromServer() {
    const bounds = map.getBounds(); // 获取当前地图视图的范围
    const southWest = bounds.getSouthWest(); // 西南角坐标
    const northEast = bounds.getNorthEast(); // 东北角坐标
    console.log('当前地图视图范围:', southWest.lng, southWest.lat, northEast.lng, northEast.lat);
    
    if (northEast.lat - southWest.lat < VIEW_SIZE) {
        const newChunks = new Set(); // 使用集合来存储当前视野内的块

        // 计算当前视野内的块
        for (let lat = Math.floor(Math.max(minlat, southWest.lat) / CHUNK_SIZE) * CHUNK_SIZE; lat <= Math.floor(Math.min(maxlat, northEast.lat) / CHUNK_SIZE) * CHUNK_SIZE; lat += CHUNK_SIZE) {
            for (let lng = Math.floor(Math.max(minlon, southWest.lng) / CHUNK_SIZE) * CHUNK_SIZE; lng <= Math.floor(Math.min(maxlon, northEast.lng) / CHUNK_SIZE) * CHUNK_SIZE; lng += CHUNK_SIZE) {
                const chunkId = getChunkId(lat, lng);
                newChunks.add(chunkId);

                // 在每个块的左下角添加文本标注经纬度
                const latitude = lat.toFixed(6);
                const longitude = lng.toFixed(6);
                const label = L.divIcon({
                    className: 'text-label',
                    html: `纬度: ${latitude}, 经度: ${longitude}`,
                    iconSize: [100, 40]
                });
                L.marker([lat, lng], { icon: label }).addTo(map);
            }
        }

        //查找需要请求的新块
        const chunksToRequest = [...newChunks].filter(chunkId => !loadedChunks[chunkId]); // 使用哈希表检查块

        //只请求未获取过的块的数据
        if (chunksToRequest.length > 0) {
            console.log("发出请求: ", chunksToRequest);

            //针对每个块分别请求
            chunksToRequest.forEach(chunkId => {
                const [chunkX, chunkY] = chunkId.split('_').map(Number);
                console.log("请求块", chunkId, chunkX, chunkY);
                loadedChunks[chunkId] = true; // 设置为 true 表示已加载

                const data = {
                    task: "render",
                    chunkId: chunkId
                }

                const jsonData = JSON.stringify(data);

                // 发送单个块的请求
                fetch('http://localhost:8080', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                        'Accept': 'application/json',
                        'Access-Control-Allow-Credentials': 'true'
                    },
                    body: jsonData
                })
                .then(response => response.json())
                .then(data => {
                    console.log('Success:', data);
                    
                    //此处处理成功返回的数据并绘制
                    const wayPolylines = []; // 存储绘制的路径
                    if (data["ways_data"] && Array.isArray(data["ways_data"])) {
                        data.ways_data.forEach(way => {
                            let latlngs = way.nodes.map(node => [node.lat, node.lng]).filter(coord => coord);
                            
                            //只在有有效坐标的情况下绘制路径
                            if (way.tags && way.tags.highway && latlngs.length > 0) {
                                const polyline = L.polyline(latlngs, { color: 'yellow' }).addTo(map); // 绘制路径
                                wayPolylines.push(polyline); // 保存到数组中
                            }
                        });
                    }

                    //将绘制的图形存储到对象中
                    drawnItems[chunkId] = wayPolylines;

                    //标记请求的块为已加载
                    console.log("已加载的块", Object.keys(loadedChunks));
                })
                .catch((error) => {
                    console.error('Error:', error);
                });
            });
        }
    }
}


// 监听地图视图变化事件，以便在用户缩放或平移地图时请求数据
map.on('moveend', requestDataFromServer);
// 绘制数据到前端地图
function drawDataOnMap(data) {
    // 假设data包含两个数组: nodes_data 和 ways_data
    // if (data["nodes_data"] && Array.isArray(data["nodes_data"])) {
    //     data["nodes_data"].forEach(node => {
    //         L.marker([node.lat, node.lng]).addTo(map); // 为每个节点绘制标记
    //     });
    // } else {
    //     console.log('无有效节点数据');
    // }

    if (data["ways_data"] && Array.isArray(data["ways_data"])) {
        data.ways_data.forEach(way => {
             
            let latlngs = way.nodes.map(node =>{
                return [node.lat, node.lng];    
            }).filter(coord => coord);

            // 只在有有效坐标的情况下绘制路径
            if (way.tags && way.tags.highway && latlngs.length > 0) {
                L.polyline(latlngs, { color: 'yellow' }).addTo(map); // 绘制路径
            }
        });
    } else {
        console.log('无有效路径数据');
    }
}


// 监听地图视图变化事件，以便在用户缩放或平移地图时请求数据
map.on('moveend', requestDataFromServer);
