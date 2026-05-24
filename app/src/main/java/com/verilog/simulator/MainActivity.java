package com.verilog.simulator;
import android.os.Bundle;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebChromeClient;
import androidx.appcompat.app.AppCompatActivity;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends AppCompatActivity {
    private static final String ASSET_DIR = "web"; // 目标子目录名
    private static final String INDEX_FILE = "index.html";
    private static final String JS_FILE = "phy_engine.js";
    private static final String WASM_FILE = "phy_engine.wasm";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        // 复制 assets 中的文件到私有目录
        copyAssetToInternal(INDEX_FILE);
        copyAssetToInternal(JS_FILE);
        copyAssetToInternal(WASM_FILE);
        
        // 获取复制后的 index.html 路径
        String baseDir = new File(getFilesDir(), ASSET_DIR).getAbsolutePath();
        String indexPath = "file://" + baseDir + "/" + INDEX_FILE;
        
        WebView webView = findViewById(R.id.webView);
        webView.getSettings().setJavaScriptEnabled(true);
        webView.getSettings().setAllowFileAccessFromFileURLs(true);
        webView.getSettings().setAllowUniversalAccessFromFileURLs(true);
        webView.setWebViewClient(new WebViewClient());
        webView.setWebChromeClient(new WebChromeClient());
        webView.loadUrl(indexPath);
    }
    
    private void copyAssetToInternal(String fileName) {
        try {
            File targetDir = new File(getFilesDir(), ASSET_DIR);
            if (!targetDir.exists()) {
                targetDir.mkdirs();
            }
            File targetFile = new File(targetDir, fileName);
            if (targetFile.exists()) {
                return; // 已存在，不需要重复复制
            }
            InputStream is = getAssets().open(fileName);
            OutputStream os = new FileOutputStream(targetFile);
            byte[] buffer = new byte[8192];
            int length;
            while ((length = is.read(buffer)) > 0) {
                os.write(buffer, 0, length);
            }
            os.close();
            is.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}