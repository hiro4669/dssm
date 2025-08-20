/**
 * utility.hpp
 * 2019/12/19 R.Gunji
 * 複数のインスタンスで使用するAPI群
 */

#ifndef _INC_DSSM_UTILITY_
#define _INC_DSSM_UTILITY_

#include <libssm.h>
#include <iostream>
#include <type_traits>
#include <utility>
#include <stdio.h>
#include <mutex>
#include <vector>

namespace dssm
{
	namespace util
	{

		/**
		 * 16byteずつhexdumpする。
		 */
		void hexdump(char *p, uint32_t len);

		/**
		 * 送信時のパケットの大きさを決めるためにインスタンス生成時に呼ばれる。
		 * thrd_msgの構造が変化したらここを変更すること。
		 */
		uint32_t countThrdMsgLength();

		/**
		 * 送信時のパケットの大きさを決めるためにインスタンス生成時に呼ばれる。
		 * ssm_msgの構造が変化したらここを変更すること。
		 */
		uint32_t countDssmMsgLength();

	} // namespace util

	namespace rbuffer
	{
		template <typename T>
		class RingBuffer
		{
		public:
			// コンストラクタ　16でバッファサイズが指定される
			inline RingBuffer() : buffer_size_(0), buf_tid(0), write_pointer_(0)
			{
				setBufferSize(16);
			}

			// コンストラクタ(リングバッファのサイズを指定(2の冪乗が最適))
			inline RingBuffer(unsigned int buffer_size_in)
			{
				// リングバッファのサイズを指定、メモリ上にバッファを確保
				setBufferSize(buffer_size_in);
			}
			// デストラクタ
			inline ~RingBuffer()
			{
			}

			void reset() {
				this->buffer_size_ = 0;
				this->buf_tid = 0;
				this->write_pointer_ = 0;
			}

			// リングバッファのサイズを指定(0より大の整数、2の冪乗が最適)
			void setBufferSize(unsigned int buffer_size_in)
			{
				if (buffer_size_in > 0)
				{
					// 排他　ミューテックスを取得することで排他処理が行われる
					// リングバッファのサイズを保持
					buffer_size_ = buffer_size_in;
					// 内部変数の初期化
					buf_tid = -1;
					write_pointer_ = 0;
					// メモリ上にリングバッファを確保 Vectorのresizeを使う
					data_.resize(buffer_size_in);
					time_data_.resize(buffer_size_in);
				}
				else
				{
					fprintf(stderr, "Error: [RingBuffer] Please set buffer size more than 0.\n");
				}
			}
			// リングバッファのサイズを取得
			inline unsigned int getBufferSize()
			{
				// 排他
				return buffer_size_;
			}

			//========Write========
			//時刻用リングバッファに書き込み
			bool writeTime(SSM_tid TID_in, ssmTimeT time_in)
			{
				// リングバッファが確保されている場合のみ処理
				if (buffer_size_ > 0)
				{
					time_data_[TID_in % buffer_size_] = time_in;
					return true;
				}
				else
				{
					fprintf(stderr, "TIME Error: [RingBuffer] Buffer is not allocated.\n");
					return false;
				}
			}
			// リングバッファにデータを書き込み
			void writeBuffer(T data_in)
			{
				// リングバッファが確保されている場合のみ処理
				if (buffer_size_ > 0)
				{
					// 排他
					// データを書き込み
					data_[write_pointer_] = data_in;
					// 書き込みポインタを進める
					write_pointer_++;
					write_pointer_ %= buffer_size_;
					// 書き込んだデータ数を保持
					buf_tid++;
				}
				else
				{
					fprintf(stderr, "DATA Error: [RingBuffer] Buffer is not allocated.\n");
				}
			}
			//　リングバッファにTIDを指定してデータを書き込み
			void writeBuffer(T data_in, SSM_tid TID_in)
			{
				write_pointer_ = TID_in;
				write_pointer_ %= buffer_size_;
				buf_tid = TID_in - 1;
				writeBuffer(data_in);
			}
			//　リングバッファにTIDと時刻を指定してデータを書き込み
			void writeBuffer(T data_in, SSM_tid TID_in, ssmTimeT time_in)
			{
				writeTime(TID_in, time_in);
				writeBuffer(data_in, TID_in);
			}

			//時刻データをTID指定して読み込み
			ssmTimeT readTime(int read_pointer_in)
			{
				uint read_pointer = read_pointer_in % buffer_size_;
				return time_data_[read_pointer];
			}

			int getTID(ssmTimeT time_in, SSM_tid &tid_r)
			{
				SSM_tid tid;
				SSM_tid top = getTID_top();
				ssmTimeT top_time = readTime(top);
				if (time_in > top_time)
				{
					tid_r = top;
					return SSM_ERROR_FUTURE;
				}
				ssmTimeT bottom = getTID_bottom();
				if (time_in < readTime(bottom))
				{
					tid_r = -2;
					return SSM_ERROR_PAST;
				}
				ssmTimeT cycle = top_time - readTime(top - 1); // cycleをTOPとその手前の差で計算する
				tid = top + (SSM_tid)((time_in - top_time) / cycle);
				if (tid > top)
					tid = top;
				else if (tid < bottom)
					tid = bottom;
				ssmTimeT itime = readTime(tid);
				while (itime < 0 || itime < time_in)
				{
					tid++;
					itime = readTime(tid);
				}
				while (itime < 0 || itime > time_in)
				{
					tid--;
					itime = readTime(tid);
				}
				if (itime < 0)
				{
					tid_r = tid;
					return -1;
				}
				else
				{
					tid_r = tid;
					return 1;
				}
			}

			SSM_tid getTID_top()
			{
				return buf_tid;
			}
			//このバッファ上での最古のTIDを返す
			SSM_tid getTID_bottom()
			{
				// TIDが-1以下だと何もデータが書かれてないことを意味するので、-1を返す
				if (buf_tid < 0)
					return buf_tid;
				//今のTIDからバッファサイズを引いて、書き込み用の1と-1のための1を足す
				int tid = buf_tid - buffer_size_ + 1 + 1;
				//最も古いTIDがマイナスになったので0を返す
				if (tid < 0)
					return 0;
				//例外はなかったので最古のTIDを返す
				return tid;
			}

			bool readNext()
			{
				return false;
			}
			// リングバッファのデータを読み込み(buf_tid: 最新, buf_tid - size + 1 + 1 : 最古)
			int read(int read_pointer_in, T &data_in, SSM_tid &tid_in, ssmTimeT &time_in)
			{
				// 過去にデータが書き込まれていないデータのポインタにはアクセスさせない
				if (read_pointer_in < 0)
					read_pointer_in = buf_tid;
				//範囲チェック
				SSM_tid top = getTID_top();
				SSM_tid bottom = getTID_bottom();
				if (read_pointer_in < SSM_TID_SP)
					return SSM_ERROR_NO_DATA;
				if (read_pointer_in > top)
					return SSM_ERROR_FUTURE;
				if (read_pointer_in < bottom)
					return SSM_ERROR_PAST;
				// 排他
				time_in = readTime(read_pointer_in);
				if (time_in < 0)
					return SSM_ERROR_NO_DATA;
				tid_in = read_pointer_in;
				uint read_pointer = read_pointer_in % buffer_size_;
				// 現在の最新データのポインタはBUF_TID, 古いほど値が小さくなる(最小: BUF_TID-BUFFER_SIZE)
				data_in = data_.at(read_pointer);
				return 1;
			}

			bool readTime()
			{
				return false;
			}

		private:
			// リングバッファのポインタ
			std::vector<T> data_;
			// タイムスタンプのポインタ
			std::vector<ssmTimeT> time_data_;
			// リングバッファのサイズ
			unsigned int buffer_size_;
			// データの数と位置をカウントするためのTID
			int buf_tid;
			// 書き込みポインタ(最新データのポインタ)
			unsigned int write_pointer_;
		};

	} // namespace rbuffer
} // namespace dssm

#endif // _INC_DSSM_UTILITY_
