const path = require("path");
const HtmlWebpackPlugin = require("html-webpack-plugin");
const MonacoWebpackPlugin = require("monaco-editor-webpack-plugin");
const WebpackMode = require("webpack-mode");

module.exports = {
    mode: `${WebpackMode}`,
    entry: "./index.js",
    output: {
        filename: "[name].bundle.js",
        path: path.resolve(__dirname, "dist")
    },
    plugins: [
        new HtmlWebpackPlugin({
            title: "VTKWasmStudio",
        }),
        new MonacoWebpackPlugin(),
    ],
    module: {
        rules: [{
            test: /\.css$/,
            use: ["style-loader", "css-loader"]
        },{
            test: /\.worker\.m?js$/,
            exclude: /monaco-editor/,
            use: ["worker-loader"],
        }]
    },
    devServer: {
        allowedHosts: "auto",
        port: "auto",
        server: "http",
    },
};
