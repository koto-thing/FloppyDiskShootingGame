"""TCPの各方向へ40〜65msの遅延を加える実通信テスト用プロキシ"""

import asyncio
import sys
from pathlib import Path


async def relay(reader, writer, jitter=False):
    """@brief 順序と帯域を保って受信を遅延配信する
    @param reader 入力ストリーム
    @param writer 出力ストリーム
    @param jitter 下り方向の周期的な遅延を許可するか
    @return なし
    """
    queue = asyncio.Queue(maxsize=256)
    loop = asyncio.get_running_loop()

    async def collect():
        """@brief パケットごとの揺らぎを予約する @return なし"""
        sequence = 0
        last_burst = -1
        while data := await reader.read(65536):
            # 指定ファイルの作成後はTCPを開いたまま通信だけを途絶させる
            if len(sys.argv) > 3 and Path(sys.argv[3]).exists():
                continue
            # 受信と遅延を別タスクにして、遅延を帯域制限へ変えない
            delay = 0.040 + (sequence % 6) * 0.005
            # 実経路で観測した200ms超の入力到着を再現し、マーカー削除で回復させる
            if jitter and len(sys.argv) > 3 and Path(sys.argv[3] + ".jitter").exists() and sequence % 24 == 0:
                delay += 0.120
            # 旧版の2秒ごとの縮小で余裕が不足する、3秒間隔の揺らぎも再現する
            burst = int(loop.time() / 3)
            if jitter and len(sys.argv) > 3 and Path(sys.argv[3] + ".bursts").exists() and burst != last_burst:
                delay += 0.120
                last_burst = burst
            await queue.put((loop.time() + delay, data))
            sequence += 1
        await queue.put((loop.time(), b""))

    async def deliver():
        """@brief 予約順に配信してTCPの順序を保つ @return なし"""
        while True:
            due, data = await queue.get()
            if not data:
                return
            await asyncio.sleep(max(0, due - loop.time()))
            writer.write(data)
            await writer.drain()

    async with asyncio.TaskGroup() as group:
        group.create_task(collect())
        group.create_task(deliver())


async def connect(reader, writer):
    """@brief ループバックのテストサーバーへ中継する
    @param reader クライアント入力
    @param writer クライアント出力
    @return なし
    """
    remote_writer = None
    tasks = []
    try:
        remote_reader, remote_writer = await asyncio.open_connection("127.0.0.1", int(sys.argv[2]))
        tasks = [asyncio.create_task(relay(reader, remote_writer)), asyncio.create_task(relay(remote_reader, writer, True))]
        await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
    except (OSError, ConnectionError):
        pass
    finally:
        # 片方向の切断で両方のタスクと接続を回収する
        for task in tasks:
            task.cancel()
        await asyncio.gather(*tasks, return_exceptions=True)
        writer.close()
        if remote_writer is not None:
            remote_writer.close()


async def main():
    """@brief 指定されたローカルポートで待機する @return なし"""
    server = await asyncio.start_server(connect, "127.0.0.1", int(sys.argv[1]))
    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(main())
