
var app = new Vue({
    el: '#app',
    data: {
        arg1: {
            keyType: "integer",
            valueSize: "4",
            constructType: "once",
            lookupType: "cacheFriendly",
        },
        arg2: {
            keyType: "short_string",
            valueSize: "4",
            constructType: "sporadic",
            lookupType: "sporadic",
        },
    },
    methods: {
        refreshChart(chartNum) {
            let arg = chartNum == 1 ? this.arg1 : this.arg2;
            console.log(arg.keyType);
            console.log(arg.valueSize);
            console.log(arg.constructType);
            console.log(arg.lookupType);
            drawAllCharts(chartNum, arg);
        },
    },
    computed: {
    }
})
