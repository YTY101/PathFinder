var startMarker = null;
var endMarker = null;
var currentPrefix = null;
var currentPolyline = null;
var currentSuffix = null;

function drawPath(data){
    // 提取经纬度数组
    var latLngs = data["path"].map(function(location) {
        return [location.lat, location.lng];
    });

    clear(); // 清除之前的路径
    // 在地图上添加连线前，检查是否已有 polyline
    currentPolyline = L.polyline(latLngs, { color: 'blue', weight: 5, pane: "markerPane" }).addTo(map);

    var Prefix_latLngs = [[data["startNodeLat"], data["startNodeLon"]], [data["startNeighborLat"], data["startNeighborLon"]]];
    currentPrefix = L.polyline(Prefix_latLngs, { color: 'red', weight: 5, zIndex: 1000 }).addTo(map);

    var Suffix_latLngs = [[data["endNodeLat"], data["endNodeLon"]], [data["endNeighborLat"], data["endNeighborLon"]]];
    currentSuffix = L.polyline(Suffix_latLngs, { color: 'red', weight: 5, zIndex: 1000 }).addTo(map);
}

var currentTarget = null;
function drawTarget(data){
    // 提取经纬度数组
    var latLngs = data["target"].map(function(location) {
        return [location.lat, location.lng];
    });

    clear(); // 清除之前的路径
    // 在地图上添加连线前，检查是否已有 polyline
    currentTarget = L.polyline(latLngs, { color: 'purple', weight: 5, pane: "markerPane" }).addTo(map);
    // 设置地图视野中心为第一个点的经纬度
    if (latLngs.length > 0) {
        map.setView(latLngs[0], map.getZoom()); // 将视野中心设置为第一个点，同时保持当前缩放级别
    }
}

function clear(){
     // 在地图上添加连线前，检查是否已有 polyline
    if (currentPolyline) {
        // 如果已存在 polyline，则从地图上移除
        map.removeLayer(currentPolyline);
        currentPolyline = null; // 清空 currentPolyline 变量
    }

    if(currentPrefix){
        map.removeLayer(currentPrefix);
        currentPrefix = null; // 清空 currentPrefix 变量
    }

    if(currentSuffix){
        map.removeLayer(currentSuffix);
        currentSuffix = null; // 清空 currentSuffix 变量
    }
    
    if(currentTarget){
        map.removeLayer(currentTarget);
        currentTarget = null; // 清空 currentTarget 变量
    }

}

var Buttons = L.Control.extend({
    // 添加控件到地图时调用
    onAdd: function (map) {
        var button_container = L.DomUtil.create('div', 'button-container');
        button_container.id = "button-container";

        var start_btn = L.DomUtil.create('button', 'start-btn', button_container);
        start_btn.id = "start-btn";
        start_btn.innerHTML = "START";

        var end_btn = L.DomUtil.create('button', 'end-btn', button_container);
        end_btn.id = "end-btn";
        end_btn.innerHTML = "END";

        // var test_btn = L.DomUtil.create('button', 'test-btn', button_container);
        // test_btn.id = "test-btn";
        // test_btn.innerHTML = "TEST";
        
        // L.DomEvent.on(test_btn, 'click', function (e) {
        //     e.stopPropagation(); // 阻止事件冒泡
        //     console.log('Test');

        //     //测试逻辑
        //     const data = {
        //         task: "path",
        //         startNodeId: "558631422",
        //         endNodeId: "475853969"
        //     }

        //     const jsonData = JSON.stringify(data);

        //     console.log('发送测试数据:', jsonData);
        //     // 发送单个块的请求
        //     fetch('http://localhost:8080', {
        //         method: 'POST',
        //         headers: {
        //             'Content-Type': 'application/json'
        //         },
        //         body: jsonData
        //     })
        //     .then(response => response.json())
        //     .then(data => {
        //         console.log('Success:', data);
        //         drawPath(data); // 调用 drawPath 函数绘制路径
        //     })
        //     .catch((error) => {
        //         console.error('Error:', error);
        //     });
        // });

        L.DomEvent.on(start_btn, 'click', function (e) {
            e.stopPropagation(); // 阻止事件冒泡
            console.log('Start');
            selecting_start = true;
            selecting_end = false; // 测试逻辑
            update();
        });
        L.DomEvent.on(end_btn, 'click', function (e) {
            e.stopPropagation(); // 阻止事件冒泡
            console.log('End');
            selecting_end = true;
            selecting_start = false;
            update();
            
        });
        return button_container;
    },

    // 从地图移除控件时调用
    onRemove: function (map) {
        // 在控件被移除时执行的操作
    }
});

// 创建控件实例并添加到地图
var button_container = new Buttons();
button_container.addTo(map);


// 创建搜索控件
var SearchControl = L.Control.extend({
    onAdd: function (map) {
        // 创建一个容器
        var container = L.DomUtil.create('div', 'search-container');
        container.id = "search-container";

        // 创建搜索输入框
        var searchInput = L.DomUtil.create('input', 'search-input', container);
        searchInput.placeholder = "搜索地点...";

        // 处理搜索输入事件
        L.DomEvent.on(searchInput, 'keypress', function (e) {
            e.stopPropagation();
            if (e.key === 'Enter') {
                var query = searchInput.value;
                // console.log('Searching for:', query);
                // 在这里添加搜索功能逻辑
                const search_data = {
                    task: "search_location",
                    query: query
                }
                const jsonSearchData = JSON.stringify(search_data);
                console.log('发送搜索数据:', jsonSearchData);

                // 使用fetch API发送POST请求到服务器
                fetch('http://localhost:8080', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                        'Accept': 'application/json',
                        'Access-Control-Allow-Credentials': 'true'
                    },
                    body: jsonSearchData
                })
                .then(response => response.json())
                .then(data => {
                    console.log('Success:', data);
                    drawTarget(data);
                })
                .catch((error) => {
                    console.error('Error:', error);
                });
            }
        });

        return container;
    },

    onRemove: function (map) {
        // 控件移除时的操作
    }
});

// 创建搜索控件实例并添加到地图
var searchControl = new SearchControl();
searchControl.addTo(map);




var selecting_start = false;
var selecting_end = false;


function checkSend(){
    if(path_data.startLat != -1 && path_data.startLng != -1 && path_data.endLat != -1 && path_data.endLng != -1){
        const jsonPathData = JSON.stringify(path_data);
        console.log('发送路径数据:', jsonPathData);

        // 使用fetch API发送POST请求到服务器
        fetch('http://localhost:8080', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Accept': 'application/json',
                'Access-Control-Allow-Credentials': 'true'
            },
            body: jsonPathData
        })
        .then(response => response.json())
        .then(data => {
            console.log('Success:', data);
            drawPath(data); // 调用 drawPath 函数绘制路径
        })
        .catch((error) => {
            console.error('Error:', error);
        });
    }
}

var clickStart, clickEnd; // 在函数外部声明
var isStartListenerAdded = false; // 跟踪是否添加了 start 监听器
var isEndListenerAdded = false; // 跟踪是否添加了 end 监听器

const path_data = {
    task: "select_path",
    mode: "",
    startLat: -1,
    startLng: -1,
    endLat: -1,
    endLng: -1,
}


function update() {
    if (selecting_start) {
        // 只有在 selecting_start 为 true 时才定义 clickStart
        if (!clickStart) {
            clickStart = function (e) {
                clear();
                var latlng = e.latlng;
                console.log('Start point selected:', latlng);
                                
                if(startMarker){
                    map.removeLayer(startMarker);
                }
                startMarker = L.marker(latlng).addTo(map);

                path_data.startLat = latlng.lat;
                path_data.startLng = latlng.lng;
                checkSend();

                selecting_start = false; // 禁用点击事件
                update();
            };
        }
        // 只在监听器未添加时添加监听器
        if (!isStartListenerAdded) {
            map.on('click', clickStart);
            isStartListenerAdded = true; // 标记监听器已添加
        }
    } else {
        // 在移除监听器前先检查是否已添加
        if (isStartListenerAdded) {
            map.off('click', clickStart); // 移除点击事件监听器
            isStartListenerAdded = false; // 标记监听器已移除
        }
    }

    if (selecting_end) {
        // 只有在 selecting_end 为 true 时才定义 clickEnd
        if (!clickEnd) {
            clickEnd = function (e) {
                clear();
                var latlng = e.latlng;
                console.log('End point selected:', latlng);

                if(endMarker){
                    map.removeLayer(endMarker);
                }
                endMarker = L.marker(latlng).addTo(map);            


                path_data.endLat = latlng.lat;
                path_data.endLng = latlng.lng;
                checkSend();

                selecting_end = false; // 禁用点击事件
                update();
            };
        }
        // 只在监听器未添加时添加监听器
        if (!isEndListenerAdded) {
            map.on('click', clickEnd);
            isEndListenerAdded = true; // 标记监听器已添加
        }
    } else {
        // 在移除监听器前先检查是否已添加
        if (isEndListenerAdded) {
            map.off('click', clickEnd); // 移除点击事件监听器
            isEndListenerAdded = false; // 标记监听器已移除
        }
    }
}
