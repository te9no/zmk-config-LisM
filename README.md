# LisM ZeroShift

LisM の左右にある拡張ソケットを Web UI から設定できます。各側でオフ、押し込み付きエンコーダ、4方向スティック、トラックボール（PAW3222）を選べます。初期設定は左右ともオフです。

## 書き込みとモジュール切替

1. [左右の UF2 をダウンロード](firmware/downloads/LisM-ZeroShift-r01.zip)して展開します。左手には `LisM-ZeroShift-left.uf2`、右手には `LisM-ZeroShift-right.uf2` を書き込みます。
2. 両手の電源を入れ、親機の右手をデータ通信対応 USB ケーブルで端末に接続します。[ZeroShift Web UI](https://te9no.github.io/zmk-feature-zeroshift/) を開き、「USB 接続」を選びます。
3. 「設定する側」で左側または右側を選び、「拡張ソケット」で装着するモジュールを選んで保存します。左側の設定には左右の接続が必要です。
4. モジュールを交換するときは電源を切り、USB を外して交換します。設定した側を再起動し、Web UI で「起動中」の表示を確認します。

## キーボード側で定義する項目

[config/zeroshift.json](config/zeroshift.json) に機種名、親機の側、スロット名、各モジュールの数値 ID と表示名を定義します。ID は保存済み設定でも使うため、表示名を変更するときも維持します。ファームウェアは接続時にこの定義を共通 Web UI に渡します。配線、ドライバーの起動、左右間の設定転送はこのリポジトリで実装しています。[共通仕様](https://github.com/te9no/zmk-feature-zeroshift/blob/main/docs/studio-descriptor.md)

## 検証

左右の UF2 は実機確認用です。Renode の左右2台構成で4×4の全16組について、保存と再起動後の適用を確認しました。センサー通信は模擬しており、実機の入力、USB 接続、左右間の無線通信は未検証です。[検証コード](tests/renode/)

このフォークは [4mplelab の LisM 設定](https://github.com/4mplelab/zmk-config-LisM/tree/8e1fd16cc9fcae7dd7a92494dc0d48aa482e3e45) をもとにしています。
