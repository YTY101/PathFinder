{/* <bounds minlat="30.8067000" minlon="120.8606000" maxlat="31.7270000" maxlon="122.4138000"/> */}

var minlat = 30.8067000;
var minlon = 120.8606000;
var maxlat = 31.7270000;
var maxlon = 122.4138000;

// 假设每个块的大小（以经纬度为单位）
const CHUNK_SIZE = 0.010;
// const VIEW_SIZE = 0.014;
const VIEW_SIZE = 0.04;


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

// 定义矩形的范围
var bounds = [[minlat, minlon], [maxlat, maxlon]];

// 创建并添加矩形到地图，仅显示边框
var rectangle = L.rectangle(bounds, { color: "#ff7800", weight: 2, fillOpacity: 0 }).addTo(map);



function getChunkId(lat, lng) {
    const chunkX = Math.floor(lat / CHUNK_SIZE);
    const chunkY = Math.floor(lng / CHUNK_SIZE);
    return `${chunkX}_${chunkY}`; // 以 "块X_块Y" 作为标识
}

const loadedChunks = {}; // 存储已加载的块
const drawnItems = {}; // 存储在地图上绘制的图形
const requestQueue = []; // 存储待请求的块
const processingRequests = []; // 存储正在处理的请求
const maxConcurrentRequests = 24; // 最大并发请求数量
const MAX_LOADED_CHUNKS = 5; // 最大加载块缓存数量

function requestDataFromServer() {
    requestQueue.length = 0; // 清空请求队列
    const bounds = map.getBounds(); // 获取当前地图视图的范围
    const southWest = bounds.getSouthWest(); // 西南角坐标
    const northEast = bounds.getNorthEast(); // 东北角坐标
    console.log('当前地图视图范围:', southWest.lng, southWest.lat, northEast.lng, northEast.lat);

    const newChunks = new Set(); // 用于存储当前视野内的块
    
    if (northEast.lat - southWest.lat < VIEW_SIZE) {
        // 计算当前视野内的块
        for (let lat = Math.floor(Math.max(minlat, southWest.lat) / CHUNK_SIZE - 1) * CHUNK_SIZE; 
            lat <= Math.floor(Math.min(maxlat, northEast.lat) / CHUNK_SIZE + 1) * CHUNK_SIZE; 
            lat += CHUNK_SIZE) {
            for (let lng = Math.floor(Math.max(minlon, southWest.lng) / CHUNK_SIZE - 1) * CHUNK_SIZE; 
                lng <= Math.floor(Math.min(maxlon, northEast.lng) / CHUNK_SIZE + 1) * CHUNK_SIZE; 
                lng += CHUNK_SIZE) {
                const chunkId = getChunkId(lat, lng);
                newChunks.add(chunkId);
            }
        }

        // 查找需要请求的新块
        const chunksToRequest = [...newChunks].filter(chunkId => !loadedChunks[chunkId]);

        // 将新的请求加入请求队列
        requestQueue.push(...chunksToRequest);

        // 处理加载块，维护 loadedChunks 最大长度为 40
        chunksToRequest.forEach(chunkId => {
            if (!loadedChunks[chunkId]) {
                // 记忆已加载块
                loadedChunks[chunkId] = true;

                // 如果已加载块超过最大限制，删除距离当前视野最远的块
                if (Object.keys(loadedChunks).length > MAX_LOADED_CHUNKS) {
                    // 计算每个已加载块的距离
                    const distances = Object.keys(loadedChunks).map(chunkId => {
                        const [lat, lng] = chunkId.split('_').map(Number);
                        const distance = bounds.getNorthEast().distanceTo([lat, lng]) || 0;
                        return { chunkId, distance };
                    });

                    // 找到距离当前视野最远的块
                    distances.sort((a, b) => b.distance - a.distance);
                    const farthestChunkId = distances[0].chunkId;

                    delete loadedChunks[farthestChunkId]; // 删除最远的块
                    // 同时移除绘制的图形（如果存在）
                    if (drawnItems[farthestChunkId]) {
                        drawnItems[farthestChunkId].forEach(polyline => {
                            map.removeLayer(polyline); // 从地图上移除对应的图形
                            console.log("移除绘制的图形", farthestChunkId);
                        });
                        delete drawnItems[farthestChunkId]; // 从存储中删除
                    }
                }
            }
        });

        // 尝试处理请求
        processRequests();
    }
}



function processRequests() {
    // 如果还可以发起请求，且请求池没有满
    while (processingRequests.length < maxConcurrentRequests && requestQueue.length > 0) {
        const chunkId = requestQueue.shift(); // 从队列中取出第一个请求
        const [chunkX, chunkY] = chunkId.split('_').map(Number);
        console.log("请求块", chunkId, chunkX, chunkY);
        
        const data = {
            task: "render",
            chunkId: chunkId
        };
        
        const jsonData = JSON.stringify(data);

        // 发送单个块的请求
        const request = fetch('http://localhost:8080', {
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
            const wayPolylines = []; // 存储绘制的路径
            if (data["ways_data"] && Array.isArray(data["ways_data"])) {
                data.ways_data.forEach(way => {
                    let latlngs = way.nodes.map(node => [node.lat, node.lng]).filter(coord => coord);
                    // 只在有有效坐标的情况下绘制路径
                    if (way.tags && way.tags.highway && latlngs.length > 0) {
                        const polyline = L.polyline(latlngs, { color: 'yellow' }).addTo(map); // 绘制路径
                        wayPolylines.push(polyline); // 保存到数组中
                        map.removeLayer(wayPolylines[wayPolylines.length - 1]); // 从地图上移除对应的图形
                    }
                });
            }
            // 将绘制的图形存储到对象中
            if(!drawnItems[chunkId]){
                drawnItems[chunkId] = [];
            }
            drawnItems[chunkId].push(wayPolylines);
            // 标记请求的块为已加载
            console.log("已加载的块", Object.keys(loadedChunks));
        })
        .catch((error) => {
            console.error('Error:', error);
        })
        .finally(() => {
            // 请求结束后从正在处理的请求中移除
            processingRequests.splice(processingRequests.indexOf(request), 1);
            loadedChunks[chunkId] = true; // 标记块为已加载
            processRequests(); // 继续处理新的请求
        });

        // 将请求添加到正在处理的请求列表
        processingRequests.push(request);
    }
}

// 监听地图视图变化事件，以便在用户缩放或平移地图时请求数据
// map.on('moveend', requestDataFromServer);


// // 绘制数据到前端地图
// function drawDataOnMap(data) {
//     // 假设data包含两个数组: nodes_data 和 ways_data
//     // if (data["nodes_data"] && Array.isArray(data["nodes_data"])) {
//     //     data["nodes_data"].forEach(node => {
//     //         L.marker([node.lat, node.lng]).addTo(map); // 为每个节点绘制标记
//     //     });
//     // } else {
//     //     console.log('无有效节点数据');
//     // }

//     if (data["ways_data"] && Array.isArray(data["ways_data"])) {
//         data.ways_data.forEach(way => {
             
//             let latlngs = way.nodes.map(node =>{
//                 return [node.lat, node.lng];    
//             }).filter(coord => coord);

//             // 只在有有效坐标的情况下绘制路径
//             if (way.tags && way.tags.highway && latlngs.length > 0) {
//                 L.polyline(latlngs, { color: 'yellow' }).addTo(map); // 绘制路径
//             }
//         });
//     } else {
//         console.log('无有效路径数据');
//     }
// }

