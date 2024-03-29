/*!
 * @brief	ブルーム。
 */
#ifndef _TKBLOOM_H_
#define _TKBLOOM_H_

namespace tkEngine{
	/*!
	 * @brief	Bloom
	 */
	class CBloomRender{
	public:
		/*!
		 * @brief	コンストラクタ。
		 */
		CBloomRender();
		/*!
		 * @brief	デストラクタ。
		 */
		~CBloomRender();
		/*!
		 * @brief	描画。
		 */
		void Render( CRenderContext& renderContext, CPostEffect* postEffect );
		/*!
		 * @brief	作成。
		 */
		void Create( const SGraphicsConfig& config );
	private:
		/*!
		* @brief	ガウスフィルタの重みを更新。
		*/
		void UpdateWeight(float dispersion);
	private:
		static const int NUM_WEIGHTS = 8;
		CEffect*		m_pEffect;						//!<エフェクト。
		CRenderTarget	m_luminanceRenderTarget;		//!<輝度を抽出するレンダリングターゲット。
		CRenderTarget	m_downSamplingRenderTarget[4];	//!<輝度をダウンサンプリングするためのレンダリングターゲット。
		bool			m_isEnable;						//!<有効？
		float				m_weights[NUM_WEIGHTS];
	};
}

#endif // _TKBLOOM_H_